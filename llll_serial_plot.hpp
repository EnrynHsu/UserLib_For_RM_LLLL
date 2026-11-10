/**
 * @file    llll_serial_plot.hpp
 * @author  llll
 * @date    2026-11-09
 * @brief   模块说明
 */

#ifndef LLLL_SERIAL_PLOT_HPP
#define LLLL_SERIAL_PLOT_HPP

// #include "stm32f1xx_hal.h"
#include "llll_uart.hpp"
#include <algorithm>
#include <initializer_list>

namespace llll
{
namespace SerialPlot
{

/**
 * @brief 串口绘图的模板类
 *
 * @tparam n 通道数
 * @tparam r 缓冲区大小（字节）
 * @tparam T 通道的数据类型
 */
template <uint8_t n, uint8_t r, typename T> class Driver : UART::Driver
{

  public:
    struct CHN
    {
        T *var;
        const char *name;
    };

    Driver(UART_HandleTypeDef *huart, uint8_t frame_header, std::initializer_list<CHN> items)
        : UART::Driver(huart, r, rx_buffer, nullptr), chn_cnt(items.size()), frame_header(frame_header)
    {
        std::copy_n(items.begin(), items.size(), chn_data);
        Start_Receive();
    };

    /**
     * @brief 组装+上传 数据帧
     *
     */
    void Update()
    {
        for (uint8_t i = 0; i < chn_cnt; i++)
        {
            std::memcpy(&tx_buffer[1 + sizeof(T) * i], chn_data[i].var, sizeof(T));
        }
        Send_Data(tx_buffer, sizeof(tx_buffer));
    };

    

  private:
    uint8_t chn_cnt;
    CHN chn_data[n]{};
    uint8_t frame_header{0xAB};
    uint8_t tx_buffer[1 + n * sizeof(T)] = {frame_header};
    uint8_t rx_buffer[r]{};

    /**
     * @brief 解析+修改 数据
     *
     * @param Size 不定长接收长度
     */
    void OnReceive(uint16_t Size) override
    {
        auto p = reinterpret_cast<char *>(rx_buffer);

        rx_buffer[Size] = '\0';

        auto eq_pos = strchr(p, '=');
        if (eq_pos == nullptr)
        {
            return;
        }
        auto hash_pos = strchr(eq_pos, '#');
        if (hash_pos == nullptr)
        {
            return;
        }
        *eq_pos = '\0';
        *hash_pos = '\0';
        // auto name = rx_buffer;
        char *endptr;
        auto value = strtod(eq_pos + 1, &endptr);
        if (endptr != hash_pos)
        {
            return;
        }

        for (uint8_t i = 0; i < chn_cnt; i++)
        {
            if (strcmp(chn_data[i].name, p) == 0)
            {
                *chn_data[i].var = static_cast<T>(value);
                return;
            }
        }
    };
};

} // namespace SerialPlot
} // namespace llll

#endif // LLLL_SERIAL_PLOT_HPP
