/**
 * @file:   llll_uart.cpp
 * @author: llll
 * @date:   2026-11-6
 */

#include "llll_uart.hpp"

namespace llll
{
namespace UART
{

/**
 * @brief   发送数据帧
 *
 * @param   pData   被发送的数据数组指针
 * @param   Size    长度
 */
HAL_StatusTypeDef Driver::Send_Data(uint8_t *pData, uint16_t Size)
{
    return HAL_UART_Transmit_DMA(huart, pData, Size);
}

uint8_t *Driver::Get_Data()
{
    return rx_buffer;
}

HAL_StatusTypeDef Driver::Start_Receive()
{
    return HAL_UARTEx_ReceiveToIdle_DMA(huart, rx_buffer, rx_buffer_length);
}

} // namespace UART

} // namespace llll
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    using namespace llll::UART;
    for (uint8_t i = 0; i < Driver::driverCount; i++)
    {
        if (huart == Driver::drivers[i]->huart)
        {

            Driver::drivers[i]->OnReceive(Size);
            HAL_UARTEx_ReceiveToIdle_DMA(huart, Driver::drivers[i]->rx_buffer, Driver::drivers[i]->rx_buffer_length);
            break;
        }
    }
}