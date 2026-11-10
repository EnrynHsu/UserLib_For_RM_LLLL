/**
 * @file    llll_gpio.hpp
 * @author  llll
 * @date    2026-11-08
 * @brief   模块说明
 */

#ifndef LLLL_GPIO_HPP
#define LLLL_GPIO_HPP

#include "stm32f1xx_hal.h"
#include <algorithm>
#include <initializer_list>

namespace llll
{
namespace GPIO
{

class Pin
{
  private:
    GPIO_TypeDef *port;
    uint16_t num;

  public:
    Pin(GPIO_TypeDef *port, uint16_t num) : port(port), num(num) {};
    Pin() = default;
    void Set();
    void Reset();
    void Toggle();
};

template <uint8_t n, typename T> class PinGrp
{
  private:
    uint8_t cnt;
    Pin pins[n];
    T state;

  public:
    PinGrp(std::initializer_list<Pin> pinList) : cnt((pinList.size() < n) ? pinList.size() : n)
    {
        std::copy_n(pinList.begin(), cnt, pins);
    };
    void Write(T state)
    {
        for (uint8_t i = 0; i < cnt; i++)
        {
            if ((state & (1 << i)) != 0)
            {
                pins[i].Set();
            }
            else
            {
                pins[i].Reset();
            }
        }
    };
};

} // namespace GPIO
} // namespace llll

#endif // LLLL_GPIO_HPP