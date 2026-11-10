/**
 * @file    llll_uart.hpp
 * @author  llll
 * @date    2026-11-08
 * @brief   模块说明
 */

#ifndef LLLL_UART_HPP
#define LLLL_UART_HPP

#include "stm32f1xx_hal.h"

namespace llll
{
namespace UART
{

/**
 * @brief UART模块自定义接口类
 */
class Driver
{
  public:
    /**
     * @brief 定义接收回调函数的类型
     *
     */
    typedef void (*RxCallback)(uint16_t Size);

    /**
     * @brief Construct a new Driver object
     *
     * @param huart uart句柄指针
     * @param rx_buffer_length 接收缓冲区数组的长度
     * @param rx_buffer 接收缓冲区数组指针
     * @param rx_call_back 接收的回调函数
     */
    Driver(UART_HandleTypeDef *huart, uint8_t rx_buffer_length, uint8_t *rx_buffer, RxCallback rx_call_back)
        : huart(huart), rx_buffer_length(rx_buffer_length), rx_buffer(rx_buffer), rx_call_back(rx_call_back)
    {
        drivers[driverCount] = this;
        driverCount++;
    };
    ~Driver() = default;

    /**
     * @brief 发送数据
     *
     * @param pData 发送数据的数组的指针
     * @param Size 数组大小
     * @return HAL_StatusTypeDef
     */
    HAL_StatusTypeDef Send_Data(uint8_t *pData, uint16_t Size);

    /**
     * @brief 拿到接收数据缓冲区的指针
     *
     * @return uint8_t*
     */
    uint8_t *Get_Data();

    HAL_StatusTypeDef Start_Receive();

  protected:
    virtual void OnReceive(uint16_t Size)
    {
        rx_call_back(Size);
    }

  private:
    static constexpr uint8_t maxDrivers = 4;
    inline static Driver *drivers[maxDrivers]{};
    inline static uint8_t driverCount{};

    UART_HandleTypeDef *huart;
    uint8_t rx_buffer_length;
    uint8_t *rx_buffer;
    RxCallback rx_call_back;

    friend void ::HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);
};

} // namespace UART
} // namespace llll

#endif // LLLL_UART_HPP