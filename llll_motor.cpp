#include "llll_motor.hpp"

namespace llll
{
namespace MOTOR
{
namespace C620
{
void Controller::OnReceive0(const CAN::Driver::RxBuffer &rxBuffer)
{
    auto id = rxBuffer.Header.StdId - 0x200 - 1;
    motors[id]->angle = (rxBuffer.Data[0] << 8) | rxBuffer.Data[1];
    motors[id]->speed = (rxBuffer.Data[2] << 8) | rxBuffer.Data[3];
    motors[id]->real_current = (rxBuffer.Data[4] << 8) | rxBuffer.Data[5];
    motors[id]->temperature = rxBuffer.Data[6];
}

void Controller::Update()
{

    uint8_t data_0[8]{};
    // 处理1~4号电机
    for (uint8_t i = 0; i < 4; i++)
    {
        if (motors[i] != nullptr)
        {
            data_0[2 * i] = static_cast<uint16_t>(motors[i]->current) >> 8;
            data_0[2 * i + 1] = static_cast<uint16_t>(motors[i]->current) & 0xFF;
        }
    }
    Send_Data(0x200, 8, data_0);
    uint8_t data_1[8]{};
    // 处理5~8号电机
    for (uint8_t i = 4; i < 8; i++)
    {
        if (motors[i] != nullptr)
        {
            data_1[2 * (i - 4)] = static_cast<uint16_t>(motors[i]->current) >> 8;
            data_1[2 * (i - 4) + 1] = static_cast<uint16_t>(motors[i]->current) & 0xFF;
        }
    }
    Send_Data(0x1FF, 8, data_1);
}
} // namespace C620

} // namespace MOTOR
} // namespace llll