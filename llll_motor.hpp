/**
 * @file    llll_motor.hpp
 * @author  llll
 * @date    2026-11-10
 * @brief   C620电机驱动
 */

#ifndef LLLL_MOTOR_HPP
#define LLLL_MOTOR_HPP

#include "llll_can.hpp"

namespace llll
{
namespace MOTOR
{

namespace C620
{

class Controller;
class Motor
{
  public:
    int16_t real_current{};
    int16_t speed{};
    int16_t angle{};
    int16_t temperature{};
    int16_t current{};

    Motor(uint8_t id) : id(id) {};
    void Set(int16_t _current)
    {
        current = _current;
    }

    void operator<<(int16_t _current)
    {
        Set(_current);
    }

  private:
    uint8_t id;

    Controller *bundled_controller{};

    friend class Controller;
};

class Controller : CAN::Driver
{
  public:
    Controller(CAN_HandleTypeDef *hcan) : CAN::Driver(hcan, nullptr, nullptr)
    {
        // 开启0号滤波器，接收0x200~0x208，发送至FIFO0接收队列
        Add_Filter_32_Mask({.STID = 0x200, .EXID = 0, .IDE = 0, .RTR = 0},
                           {.STID = 0x7F0, .EXID = 1, .IDE = 1, .RTR = 1}, 0, CAN::Driver::FIFO_Channel::FIFO0);
    };

    /**
     * @brief 绑定电机到电调
     *
     * @param motor 要绑定到此电调的电机
     */
    void Register(Motor &motor)
    {
        motors[motor.id - 1] = &motor;   // 注册到电调对象的电机列表里
        motor.bundled_controller = this; // 回绑电调（实现双向绑定）
    }

    /**
     * @brief 绑定电机到电调
     *
     * @param motor 要绑定到此电调的电机
     */
    void operator<<(Motor &motor)
    {
        Register(motor);
    }

    /**
     * @brief 打包所有电机的电流控制上传到电调
     *
     */
    void Update();

  private:
    static constexpr uint8_t maxMotors = 8;

    Motor *motors[maxMotors]{};
    uint8_t motorCount{};
    void OnReceive0(const RxBuffer &rxBuffer) override;
};

} // namespace C620

} // namespace MOTOR
} // namespace llll

#endif // LLLL_MOTOR_HPP
