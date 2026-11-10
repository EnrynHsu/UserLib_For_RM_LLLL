/**
 * @file llll_gpio.cpp
 * @author llll
 * @brief GPIO的抽象封装
 * @version 0.1
 * @date 2026-11-09
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "llll_gpio.hpp"

void llll::GPIO::Pin::Set()
{
    HAL_GPIO_WritePin(port, num, GPIO_PIN_SET);
}
void llll::GPIO::Pin::Reset()
{
    HAL_GPIO_WritePin(port, num, GPIO_PIN_RESET);
}
void llll::GPIO::Pin::Toggle()
{
    HAL_GPIO_TogglePin(port, num);
}

// template <uint8_t n, typename T> void llll::GPIO::PinGrp<n, T>::Write(T state)
// {
//     for (uint8_t i = 0; i < cnt; i++)
//     {
//         if ((state & (1 << i)) != 0)
//         {
//             pins[i].Set();
//         } else {
//             pins[i].Reset();
//         }
        
//     }
// }