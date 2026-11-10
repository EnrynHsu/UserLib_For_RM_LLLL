# LLLL STM32F1 C++ 外设库接口文档

本文档依据工作区截至 2026-11-11 的源码整理，覆盖 UART、串口波形、GPIO、CAN
和 C620 电机控制五个模块。

## 1. 库概览

本库是面向 STM32F1 HAL 的轻量 C++ 封装，命名空间统一为 `llll`。主要目标是：

- 用类封装 HAL 句柄和回调分发。
- 用模板在编译期确定通道数和缓冲区大小。
- 通过 HAL 回调把数据转发给上层。
- 保持与 CubeMX 生成的 STM32F1 HAL 工程兼容。

| 模块 | 命名空间 | 主要文件 | 用途 |
| --- | --- | --- | --- |
| UART | `llll::UART` | `llll_uart.hpp/.cpp` | DMA 发送、DMA + IDLE 接收 |
| SerialPlot | `llll::SerialPlot` | `llll_serial_plot.hpp` | 二进制波形上传、文本变量修改 |
| GPIO | `llll::GPIO` | `llll_gpio.hpp/.cpp` | GPIO 引脚和引脚组控制 |
| CAN | `llll::CAN` | `llll_can.hpp/.cpp` | bxCAN 发送、过滤和中断接收 |
| C620 | `llll::MOTOR::C620` | `llll_motor.hpp/.cpp` | C620 控制帧聚合与反馈分发 |

## 2. 环境与集成

### 2.1 编译要求

- MCU 系列：STM32F1，使用 `stm32f1xx_hal.h`。
- 需要启用对应的 HAL 模块，例如 `HAL_UART_MODULE_ENABLED`、
  `HAL_CAN_MODULE_ENABLED`。
- UART 模块依赖 DMA。
- 库使用了 `inline static` 类成员，编译器需要支持 C++17 或更高标准。
- 当前源码注释主要使用 C++ 类声明，主要公共接口位于 `.hpp` 文件。

### 2.2 文件加入工程

至少需要把以下实现文件加入构建：

```text
llll_uart.cpp
llll_gpio.cpp
llll_can.cpp
llll_motor.cpp
```

`llll_serial_plot.cpp` 目前为空，`SerialPlot` 的实现全部位于模板头文件
`llll_serial_plot.hpp` 中。

### 2.3 初始化顺序

CubeMX 生成的外设和 DMA 应先完成初始化，再构造本库对象：

```cpp
HAL_Init();
SystemClock_Config();
MX_GPIO_Init();
MX_DMA_Init();
MX_CAN_Init();
MX_USART1_UART_Init();

// 在这里构造 llll 库对象
```

### 2.4 回调函数名称

库实现了以下 HAL 回调，工程中不应再定义同名函数：

```text
HAL_UARTEx_RxEventCallback
HAL_CAN_RxFifo0MsgPendingCallback
HAL_CAN_RxFifo1MsgPendingCallback
```

这些回调运行在中断上下文。用户回调应尽量短，不建议执行阻塞延时、复杂浮点
运算、动态内存分配或长时间关闭中断。

### 2.5 对象生命周期

UART 和 CAN 的 `Driver` 会把 `this` 注册到静态分发表中。C620 的 `Controller`
还会保存已注册 `Motor` 的地址。相关对象必须具有足够长的生命周期，建议优先
使用全局对象或在 `main()` 内使用静态局部对象：

```cpp
static llll::UART::Driver uart{&huart1, sizeof(rx_buffer), rx_buffer, OnUart};
```

不要把驱动对象创建在会提前离开作用域的函数中。析构函数不会从静态分发表中
移除对象，因此临时对象或局部对象结束后，中断回调可能访问失效地址。

UART 和 CAN 各自最多保存 4 个驱动对象注册信息，当前实现没有容量溢出保护。
C620 的 `Motor` 必须比引用它的 `Controller` 生命周期更长。

## 3. UART 接口

头文件：

```cpp
#include "llll_uart.hpp"
```

### 3.1 类型

```cpp
using RxCallback = void (*)(uint16_t Size);
```

接收回调参数 `Size` 表示本次 DMA + IDLE 接收到的实际字节数，数据从
`Get_Data()` 返回的缓冲区读取。

### 3.2 构造函数

```cpp
Driver(
    UART_HandleTypeDef *huart,
    uint8_t rx_buffer_length,
    uint8_t *rx_buffer,
    RxCallback rx_call_back);
```

| 参数 | 说明 |
| --- | --- |
| `huart` | 已由 CubeMX 初始化的 UART HAL 句柄 |
| `rx_buffer_length` | 接收缓冲区长度，类型为 `uint8_t`，最大 255 字节 |
| `rx_buffer` | 用户提供的接收缓冲区，生命周期必须覆盖驱动对象 |
| `rx_call_back` | UART 接收回调；使用默认接收路径时不能为空 |

构造函数只注册驱动，不会自动启动接收。必须在所有初始化完成后调用
`Start_Receive()`。

### 3.3 `Send_Data`

```cpp
HAL_StatusTypeDef Send_Data(uint8_t *pData, uint16_t Size);
```

使用 `HAL_UART_Transmit_DMA()` 启动一次 DMA 发送。

| 参数 | 说明 |
| --- | --- |
| `pData` | 发送数据指针 |
| `Size` | 发送字节数 |
| 返回值 | `HAL_OK` 表示 DMA 发送已成功启动 |

注意事项：

- `HAL_OK` 只表示传输已经开始，不表示最后一个字节已经发送完成。
- DMA 发送期间，`pData` 指向的缓冲区必须保持有效，不能修改或释放。
- 上一次 DMA 尚未完成时再次调用可能返回忙状态。
- 需要确认 UART 的 TX DMA 通道、DMA 中断和 NVIC 已正确配置。

### 3.4 `Start_Receive`

```cpp
HAL_StatusTypeDef Start_Receive();
```

使用 `HAL_UARTEx_ReceiveToIdle_DMA()` 启动不定长接收。总线进入空闲状态或接收
缓冲区被填满时触发回调。

接收事件处理完成后，库会自动再次调用 `Start_Receive()`，因此正常情况下只需
在初始化后调用一次。

### 3.5 `Get_Data`

```cpp
uint8_t *Get_Data();
```

返回构造时传入的接收缓冲区地址。回调中的 `Size` 对应当前有效数据长度。

### 3.6 派生类与 `OnReceive`

`OnReceive()` 是受保护的虚函数：

```cpp
virtual void OnReceive(uint16_t Size);
```

默认实现调用构造时传入的 `rx_call_back`。派生类可以覆盖该函数以自行解析数据。
覆盖后可以不提供普通回调，但构造函数仍要求传入回调参数；此场景可以传
`nullptr`。

### 3.7 UART 示例

```cpp
static uint8_t uart_rx[64]{};

void OnUartReceive(uint16_t size)
{
    // 只处理前 size 个字节
    for (uint16_t i = 0; i < size; ++i)
    {
        (void)uart_rx[i];
    }
}

void SetupUart()
{
    // 在 MX_USART1_UART_Init() 和 MX_DMA_Init() 之后调用本函数
    static llll::UART::Driver uart{
        &huart1,
        sizeof(uart_rx),
        uart_rx,
        OnUartReceive
    };

    uart.Start_Receive();

    static uint8_t uart_tx[] = "hello\r\n";
    uart.Send_Data(uart_tx, sizeof(uart_tx) - 1);
}
```

## 4. SerialPlot 接口

头文件：

```cpp
#include "llll_serial_plot.hpp"
```

`SerialPlot` 用于周期性发送按通道排列的二进制波形数据，同时接收
`名称=数值#` 格式的文本命令修改指定变量。

### 4.1 模板参数

```cpp
template <uint8_t n, uint8_t r, typename T>
class Driver;
```

| 参数 | 说明 |
| --- | --- |
| `n` | 波形通道数 |
| `r` | 接收命令缓冲区大小，单位为字节 |
| `T` | 每个通道的数据类型，例如 `float`、`int16_t` |

### 4.2 通道类型

```cpp
struct CHN
{
    T *var;
    const char *name;
};
```

- `var` 指向被采集的变量，不能为空。
- `name` 是用于接收文本命令的通道名，不能为空。
- 通道变量和名称的生命周期必须覆盖 `SerialPlot::Driver` 对象。

### 4.3 构造函数

```cpp
Driver(
    UART_HandleTypeDef *huart,
    uint8_t frame_header,
    std::initializer_list<CHN> items);
```

| 参数 | 说明 |
| --- | --- |
| `huart` | 用于发送和接收的 UART 句柄 |
| `frame_header` | 每个二进制数据帧的首字节 |
| `items` | 通道列表，数量不能超过模板参数 `n` |

构造函数创建内部收发缓冲区、注册 UART 驱动并自动启动接收。

### 4.4 `Update`

```cpp
void Update();
```

抓取所有通道变量，组装一帧数据并调用 UART DMA 发送。建议在主循环中周期性调用。

发送帧格式：

```text
[frame_header]
[通道 0 的 sizeof(T) 字节]
[通道 1 的 sizeof(T) 字节]
...
[通道 n-1 的 sizeof(T) 字节]
```

当前实现直接复制 STM32 内存中的原始字节，因此：

- STM32F1 为小端字节序。
- 上位机必须使用相同的字节序和相同的数据类型宽度。
- `float` 通道需要按 IEEE-754 单精度浮点数解析。
- 每帧固定发送 `1 + n * sizeof(T)` 字节。

### 4.5 上位机修改变量

接收方向使用以下文本格式：

```text
名称=数值#
```

例如：

```text
gain=2.5#
offset=-10#
```

解析完成后，数值会被转换为模板参数 `T` 并写入对应通道变量。帧头字节只用于
上行二进制数据，不参与下行文本命令解析。

### 4.6 SerialPlot 示例

```cpp
static float x{};
static float y{};

static llll::SerialPlot::Driver<2, 64, float> serial_plot{
    &huart1,
    0xAB,
    {
        {&x, "x"},
        {&y, "y"}
    }
};

while (1)
{
    // 更新 x、y
    serial_plot.Update();
    HAL_Delay(10);
}
```

### 4.7 SerialPlot 使用约束

- `items` 中的元素数量必须小于或等于 `n`。
- 接收文本命令的长度必须严格小于 `r`。当前实现在 `rx_buffer[Size]` 写入结束符；
  当 `Size == r` 时会发生一字节越界。
- 下行命令必须完整包含名称、等号、数值和 `#`，例如 `x=1.0#`。
- `Update()` 的内部发送缓冲区由对象重复使用。上一次 UART DMA 尚未完成时再次调用
  `Update()`，可能覆盖正在发送的数据或返回 UART 忙状态。

## 5. GPIO 接口

头文件：

```cpp
#include "llll_gpio.hpp"
```

GPIO 外设时钟、引脚模式和速度仍由 CubeMX 或 HAL 初始化，本模块只负责引脚状态
操作。

### 5.1 `Pin`

```cpp
Pin(GPIO_TypeDef *port, uint16_t num);
Pin();
```

| 参数 | 说明 |
| --- | --- |
| `port` | GPIO 端口，例如 `GPIOA` |
| `num` | 引脚掩码，例如 `GPIO_PIN_5` |

成员函数：

```cpp
void Set();     // 输出高电平
void Reset();   // 输出低电平
void Toggle();  // 翻转输出电平
```

默认构造的 `Pin` 尚未绑定端口和引脚，不应直接调用上述成员函数。

### 5.2 `PinGrp`

```cpp
template <uint8_t n, typename T>
class PinGrp
{
public:
    PinGrp(std::initializer_list<Pin> pinList);
    void Write(T state);
};
```

| 参数 | 说明 |
| --- | --- |
| `n` | 最大引脚数量 |
| `T` | 状态变量类型，必须能够容纳 `n` 个状态位 |
| `pinList` | 引脚列表；超过 `n` 的部分会被忽略 |

`state` 的第 `i` 位控制 `pins[i]`：

- `1`：调用 `Set()`
- `0`：调用 `Reset()`

### 5.3 GPIO 示例

```cpp
using namespace llll::GPIO;

Pin led{GPIOC, GPIO_PIN_13};
led.Toggle();

PinGrp<4, uint8_t> output_pins{
    Pin{GPIOA, GPIO_PIN_4},
    Pin{GPIOA, GPIO_PIN_5},
    Pin{GPIOB, GPIO_PIN_10},
    Pin{GPIOB, GPIO_PIN_11}
};

// 低两位输出高电平，其余输出低电平
output_pins.Write(0x03);
```

## 6. CAN 接口

头文件：

```cpp
#include "llll_can.hpp"
```

### 6.1 公共类型

接收帧：

```cpp
struct RxBuffer
{
    CAN_RxHeaderTypeDef Header{};
    uint8_t Data[8]{};
};
```

发送帧：

```cpp
struct TxBuffer
{
    const CAN_TxHeaderTypeDef Header;
    const uint8_t *Data;

    TxBuffer(CAN_TxHeaderTypeDef header, uint8_t *data);
};
```

FIFO 选择：

```cpp
enum class FIFO_Channel : uint32_t
{
    FIFO0 = CAN_RX_FIFO0,
    FIFO1 = CAN_RX_FIFO1
};
```

过滤器描述：

```cpp
struct FilterID_32_Mask
{
    uint32_t STID;
    uint32_t EXID;
    uint32_t IDE = 0;
    uint32_t RTR = 0;
};
```

接收回调：

```cpp
using RxCallback = void (*)(const RxBuffer &);
```

### 6.2 构造函数

```cpp
Driver(
    CAN_HandleTypeDef *can_handler,
    RxCallback rx_call_back_0,
    RxCallback rx_call_back_1);
```

构造函数会：

1. 将对象注册到静态分发表。
2. 调用 `HAL_CAN_Start()` 启动 CAN。
3. 开启 FIFO0 和 FIFO1 的消息挂起中断。

必须在外设初始化 `MX_CAN_Init()` 和对应 NVIC 配置完成后构造对象。

FIFO0 和 FIFO1 的回调在中断中被直接调用，因此两个回调都应由用户保证非空。
如果不使用某个 FIFO，可以传入一个安全的空函数。

### 6.3 `Send_Data`

```cpp
HAL_StatusTypeDef Send_Data(
    uint32_t ID,
    uint32_t Size,
    uint8_t *Data);
```

发送标准数据帧，帧头固定为：

```text
StdId = ID
ExtId = 0
IDE   = CAN_ID_STD
RTR   = CAN_RTR_DATA
DLC   = Size
```

| 参数 | 说明 |
| --- | --- |
| `ID` | 11 位标准 ID，有效范围 `0x000` 到 `0x7FF` |
| `Size` | 数据长度，经典 CAN 有效范围 0 到 8 |
| `Data` | 发送数据缓冲区 |

发送数据缓冲区必须至少包含 8 个可读字节。当前 HAL 实现无论 `DLC` 是多少都会读取
`Data[0]` 到 `Data[7]`，因此不能传入单个 `uint8_t` 变量的地址：

```cpp
// 错误：底层仍会读取 data[0] 到 data[7]
uint8_t value = 0x80;
can.Send_Data(0x123, 1, &value);

// 正确
uint8_t tx_data[8] = {0x80};
can.Send_Data(0x123, 1, tx_data);
```

返回 `HAL_OK` 表示帧已写入空闲发送邮箱，不表示已经成功发送到 CAN 总线。

### 6.4 `Send_Frame`

```cpp
HAL_StatusTypeDef Send_Frame(TxBuffer tx_buffer);
```

通用发送接口，可以自行指定标准帧或扩展帧、数据帧或远程帧：

```cpp
uint8_t data[8]{0x01, 0x02};

CAN_TxHeaderTypeDef header{};
header.StdId = 0x123;
header.ExtId = 0;
header.IDE = CAN_ID_STD;
header.RTR = CAN_RTR_DATA;
header.DLC = 2;

llll::CAN::Driver::TxBuffer frame{header, data};
can.Send_Frame(frame);
```

配置扩展帧时应使用 `CAN_ID_EXT` 并填写合法的 `ExtId`。底层 HAL 仍会读取完整的
8 字节发送缓冲。

### 6.5 `Add_Filter_32_Mask`

```cpp
void Add_Filter_32_Mask(
    FilterID_32_Mask ID,
    FilterID_32_Mask MaskID,
    uint32_t Bank,
    FIFO_Channel fifo_c);
```

配置一个 32 位 ID/Mask 验收过滤器。

| 参数 | 说明 |
| --- | --- |
| `ID` | 匹配目标；由标准 ID、扩展 ID、IDE 和 RTR 字段组成 |
| `MaskID` | 掩码；对应位为 1 表示必须匹配，为 0 表示忽略 |
| `Bank` | 过滤器组编号；STM32F103 单 CAN 通常使用 0 到 13 |
| `fifo_c` | 匹配帧进入 `FIFO0` 或 `FIFO1` |

所有 ID 和掩码均为零时，会接收所有 CAN 帧：

```cpp
can.Add_Filter_32_Mask(
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    0,
    llll::CAN::Driver::FIFO_Channel::FIFO0);
```

只接收标准数据帧 ID `0x123`：

```cpp
can.Add_Filter_32_Mask(
    {0x123, 0, 0, 0},
    {0x7FF, 0, 1, 1},
    0,
    llll::CAN::Driver::FIFO_Channel::FIFO0);
```

掩码的 `IDE=1` 用于锁定标准帧/扩展帧类型，`RTR=1` 用于锁定数据帧/远程帧类型。

当前函数返回 `void`，`HAL_CAN_ConfigFilter()` 的错误状态被忽略。过滤器配置失败时
不会通过该接口报告。

### 6.6 CAN 接收回调

接收中断会：

1. 根据 `CAN_HandleTypeDef *` 找到对应 `Driver`。
2. 从 FIFO 取出一个帧到该驱动内部的 `RxBuffer`。
3. 调用对应的虚函数 `OnReceive0()` 或 `OnReceive1()`。

基类默认实现会调用构造时传入的 `RxCallback`。派生类可以覆盖
`OnReceive0()`/`OnReceive1()` 直接处理数据，此时对应回调可以传 `nullptr`。

回调中的 `RxBuffer` 引用指向驱动内部缓冲区，下一帧到达时会被覆盖。如果需要在
回调返回后继续使用，应先把数据复制到自己的缓冲区。

### 6.7 CAN 示例

```cpp
void OnCanFifo0(const llll::CAN::Driver::RxBuffer &frame)
{
    if (frame.Header.DLC == 0)
        return;

    uint8_t first_byte = frame.Data[0];
    (void)first_byte;
}

void OnCanFifo1(const llll::CAN::Driver::RxBuffer &)
{
}

void SetupCan()
{
    // 在 MX_CAN_Init() 和 NVIC 配置完成后调用本函数
    static uint8_t can_tx[8] = {0x01};

    static llll::CAN::Driver can{
        &hcan,
        OnCanFifo0,
        OnCanFifo1
    };

    can.Add_Filter_32_Mask(
        {0x123, 0, 0, 0},
        {0x7FF, 0, 1, 1},
        0,
        llll::CAN::Driver::FIFO_Channel::FIFO0);

    can.Send_Data(0x123, 1, can_tx);
}
```

本库不会修改 `hcan.Init.Mode`。静默回环模式适合板上自测，但不会在物理 CAN
总线上正常收发；连接 C620 等外部设备前必须确认工程使用 `CAN_MODE_NORMAL`
并正确配置波特率。

## 7. C620 电机控制接口

头文件：

```cpp
#include "llll_motor.hpp"
```

该模块将职责分为两层：

- `Controller` 负责 CAN 通信、控制帧聚合和反馈分发。
- `Motor` 保存电机 ID、目标电流和反馈数据。

一个 `Controller` 管理同一 CAN 控制帧组中的最多 8 个电机。电机 1～4 使用
`0x200`，电机 5～8 使用 `0x1FF`；C620 的反馈 ID 为 `0x201～0x208`。

### 7.1 `Motor`

```cpp
namespace llll::MOTOR::C620
{

class Motor
{
  public:
    int16_t real_current{};
    int16_t speed{};
    int16_t angle{};
    int16_t temperature{};
    int16_t current{};

    Motor(uint8_t id);
    void Set(int16_t current);
    void operator<<(int16_t current);
};

}
```

电机 ID 的有效范围是 1～8。`current` 是 C620 的原始转矩电流指令，范围为
`-16384～16384`，对应约 `-20A～20A`。例如：

```cpp
llll::MOTOR::C620::Motor motor{1};
motor.Set(1000);
motor << -1000;
```

反馈字段说明：

| 字段 | 含义 | 数据来源 |
| --- | --- | --- |
| `angle` | 机械角度 | 反馈数据 `Data[0..1]` |
| `speed` | 转速，单位 rpm | 反馈数据 `Data[2..3]` |
| `real_current` | 实际转矩电流原始值 | 反馈数据 `Data[4..5]` |
| `temperature` | 温度 | 反馈数据 `Data[6]` |
| `current` | 下一控制帧使用的目标电流 | 用户写入 |

### 7.2 `Controller`

```cpp
class Controller : CAN::Driver
{
  public:
    Controller(CAN_HandleTypeDef *hcan);
    void Register(Motor &motor);
    void operator<<(Motor &motor);
    void Update();
};
```

构造 `Controller` 时会：

1. 注册并启动底层 CAN 驱动。
2. 配置过滤器，接收 `0x200～0x20F` 范围内的标准数据帧并送入 FIFO0。
3. 后续由中断回调把 `0x201～0x208` 的反馈分发到对应 `Motor`。

注册电机：

```cpp
llll::MOTOR::C620::Controller controller{&hcan};
llll::MOTOR::C620::Motor motor1{1};
llll::MOTOR::C620::Motor motor2{2};

controller << motor1;
controller.Register(motor2);
```

`Update()` 组装并发送控制帧：

- 电机 1～4 写入 `0x200` 的四个 `int16_t` 槽位。
- 电机 5～8 写入 `0x1FF` 的四个 `int16_t` 槽位。
- 未注册的槽位发送零电流。
- 每帧字节序为高字节在前、低字节在后。

应在控制循环中周期调用，例如每 1～10ms 调用一次：

```cpp
while (1)
{
    controller.Update();
    HAL_Delay(10);
}
```

### 7.3 与 SerialPlot 联调

下面的例子同时上传电流、角度、转速、温度，并允许上位机修改目标电流：

```cpp
static llll::MOTOR::C620::Controller controller{&hcan};
static llll::MOTOR::C620::Motor motor{1};

controller << motor;

static llll::SerialPlot::Driver<5, 64, int16_t> serial_plot{
    &huart1,
    0xAB,
    {
        {&motor.real_current, "real_current"},
        {&motor.angle, "angle"},
        {&motor.speed, "speed"},
        {&motor.temperature, "temperature"},
        {&motor.current, "current"}
    }
};

while (1)
{
    controller.Update();
    serial_plot.Update();
    HAL_Delay(10);
}
```

上位机发送以下文本即可把目标电流改为 1000：

```text
current=1000#
```

发送负数时使用：

```text
current=-1000#
```

上位机接收的二进制通道使用 STM32F1 的小端字节序；控制帧发送给 C620 时则由
`Controller` 转换成高字节在前。

### 7.4 CAN 硬件与引脚

`Controller` 只负责 C620 协议，不负责 CAN 收发器引脚配置。STM32F1 的 CAN1
默认引脚为 `PA11/PA12`；使用 `PB8/PB9` 时必须启用 AFIO 时钟并调用：

```c
__HAL_AFIO_REMAP_CAN1_2();
```

MCU 不能把 CAN 引脚直接连接到 C620 的 `CAN_H/CAN_L`，必须经过 CAN 收发器。
还需要确认 CAN_H/CAN_L 没有接反、总线终端电阻和 C620 的波特率配置正确。

## 8. 返回值与异步行为

| 接口 | 成功返回 | 成功后的实际含义 |
| --- | --- | --- |
| `UART::Send_Data` | `HAL_OK` | DMA 发送已启动 |
| `UART::Start_Receive` | `HAL_OK` | 接收 DMA 已启动 |
| `CAN::Send_Data` | `HAL_OK` | 数据已放入 CAN 发送邮箱 |
| `CAN::Send_Frame` | `HAL_OK` | 数据已放入 CAN 发送邮箱 |
| `CAN::Add_Filter_32_Mask` | 无返回值 | 无法获知过滤配置结果 |

对 UART 和 CAN 的发送接口，上层不应把 `HAL_OK` 等同为“接收端已经收到”。

## 9. 当前实现的已知限制

以下内容是对当前源码行为的记录，不代表未来版本的接口承诺：

- UART 和 CAN 的静态驱动表最多容纳 4 个对象，写入前没有越界检查。
- UART 和 CAN 的析构函数不会注销静态分发项。
- CAN 构造函数和过滤器配置函数没有向上返回 HAL 错误状态。
- CAN 默认接收实现没有对空 `RxCallback` 进行检查；派生类覆盖回调时可以不传
  普通回调，但未覆盖对应 FIFO 时应提供安全回调。
- CAN 的 `Send_Data()` 不检查标准 ID 是否超过 `0x7FF`，也不检查 DLC 是否超过 8。
- CAN 底层发送会读取完整 8 字节缓冲区，即使 DLC 小于 8。
- `SerialPlot` 接收到长度恰好等于 `r` 的命令时会越界写入一个 `\0`。
- `SerialPlot` 构造时没有限制通道列表长度，传入的元素数不能超过模板参数 `n`。
- UART 默认接收路径没有检查构造时的回调是否为空。
- `Controller::OnReceive0()` 当前未检查反馈 ID 范围和 `motors[id]` 是否为空；
  应确保滤波器、电机注册和 ID 配置正确后再用于正式工程。
- `Controller::Register()` 当前不检查 ID 是否越界、是否重复注册或是否已绑定
  其他 Controller。
- `Controller::Update()` 当前返回 `void`，不报告控制帧是否成功写入发送邮箱。
- `Controller` 当前固定使用过滤器和 FIFO0；同一 `hcan` 不建议再创建独立
  `CAN::Driver` 或其他 C620 Controller。
- `Motor` 和 `Controller` 当前允许隐式复制；硬件对象应按唯一实例使用。

## 10. 源文件索引

- [llll_uart.hpp](llll_uart.hpp)
- [llll_uart.cpp](llll_uart.cpp)
- [llll_serial_plot.hpp](llll_serial_plot.hpp)
- [llll_gpio.hpp](llll_gpio.hpp)
- [llll_gpio.cpp](llll_gpio.cpp)
- [llll_can.hpp](llll_can.hpp)
- [llll_can.cpp](llll_can.cpp)
- [llll_motor.hpp](llll_motor.hpp)
- [llll_motor.cpp](llll_motor.cpp)
