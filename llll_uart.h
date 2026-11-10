/**
 * @file    ：      llll_uart.h
 * @author  :       llll
 * @brief   :       串口的工具文件接口
 */

#ifndef __LLLL_UART_H
#define __LLLL_UART_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ---------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
/* Exported constants -----------------------------------------------------*/

/* Exported types ---------------------------------------------------------*/
typedef void (*LLLL_Uart_RxCallbackTypeDef)(uint16_t Size);

/**
 * @brief 自定义的串口句柄结构体类型
 */
typedef struct
{
    UART_HandleTypeDef *huart;
    uint8_t rx_buffer_length;
    uint8_t *rx_buffer;
    LLLL_Uart_RxCallbackTypeDef rx_call_back;
} LLLL_UART_HandleTypeDef;

/* Exported variables -----------------------------------------------------*/

/* Exported functions -----------------------------------------------------*/
void LLLL_UART_Init(LLLL_UART_HandleTypeDef *luart);
HAL_StatusTypeDef LLLL_UART_Send_Data(uint8_t *pData, uint16_t Size);

#ifdef __cplusplus
}
#endif

#endif // __LLLL_UART_H
