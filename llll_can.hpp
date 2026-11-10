/**
 * @file    llll_can.hpp
 * @author  llll
 * @date    2026-11-07
 * @brief   CAN外设
 */

#ifndef LLLL_CAN_HPP
#define LLLL_CAN_HPP

#include "stm32f1xx_hal.h"

namespace llll
{
namespace CAN
{

/**
 * @brief 模块接口
 */
class Driver
{
  public:
    /**
     * @brief 接收缓冲区类型
     *
     */
    struct RxBuffer
    {
        CAN_RxHeaderTypeDef Header{};
        uint8_t Data[8]{};
    };

    /**
     * @brief 发送缓冲区结构体
     *
     */
    struct TxBuffer
    {

        const CAN_TxHeaderTypeDef Header;
        const uint8_t *Data;
        TxBuffer(CAN_TxHeaderTypeDef header, uint8_t *data) : Header(header), Data(data)
        {
        }

      private:
        uint32_t UsedMailbox{};

        friend class Driver;
    };

    /**
     * @brief FIFO通道枚举
     *
     */
    enum class FIFO_Channel : uint32_t
    {
        FIFO0 = CAN_RX_FIFO0,
        FIFO1 = CAN_RX_FIFO1
    };

    /**
     * @brief
     *
     */
    struct FilterID_32_Mask
    {
        uint32_t STID;    // 标准ID
        uint32_t EXID;    // 拓展ID
        uint32_t IDE = 0; // 是否开启拓展ID
        uint32_t RTR = 0; // 是否是遥控帧
    };

    using RxCallback = void (*)(const RxBuffer &);

    /**
     * @brief 应在hcan初始化后构造
     *
     * @param can_handler 使用的CAN句柄
     * @param rx_call_back_0  传入FIFO0的回调函数
     * @param rx_call_back_1  传入FIFO1的回调函数
     */
    Driver(CAN_HandleTypeDef *can_handler, RxCallback rx_call_back_0, RxCallback rx_call_back_1)
        : can_handler(can_handler), rx_call_back_0(rx_call_back_0), rx_call_back_1(rx_call_back_1)
    {
        drivers[driverCount] = this;
        driverCount++;
        HAL_CAN_Start(can_handler);
        __HAL_CAN_ENABLE_IT(can_handler, CAN_IT_RX_FIFO0_MSG_PENDING);
        __HAL_CAN_ENABLE_IT(can_handler, CAN_IT_RX_FIFO1_MSG_PENDING);
    }

    /**
     * @brief Destroy the Driver object
     *
     */
    ~Driver() = default;

    /**
     * @brief 发送帧
     *
     * @param tx_buffer 传入帧
     * @return HAL_StatusTypeDef
     */
    HAL_StatusTypeDef Send_Frame(TxBuffer tx_buffer);

    /**
     * @brief 发送标准数据帧
     *
     * @param ID 标准帧ID（11位）
     * @param Size DLC
     * @param Data 数据数组
     * @return HAL_StatusTypeDef
     */
    HAL_StatusTypeDef Send_Data(uint32_t ID, uint32_t Size, uint8_t *Data); // 发送标准数据帧

    /**
     * @brief 开启一个32位掩码模式的滤波器
     *
     * @param ID 目标ID
     * @param MaskID 掩码
     * @param Bank 滤波器序号
     * @param fifo_c FIFO通道
     */
    void Add_Filter_32_Mask(Driver::FilterID_32_Mask ID, Driver::FilterID_32_Mask MaskID, uint32_t Bank,
                            FIFO_Channel fifo_c);

  private:
    RxBuffer rxBuffer;

    static constexpr uint8_t maxDrivers = 4;
    inline static Driver *drivers[maxDrivers]{};
    inline static uint8_t driverCount{};

    CAN_HandleTypeDef *const can_handler;
    RxCallback rx_call_back_0;
    RxCallback rx_call_back_1;

    virtual void OnReceive0(const RxBuffer &rxBuffer)
    {
        rx_call_back_0(rxBuffer);
    };
    virtual void OnReceive1(const RxBuffer &rxBuffer)
    {
        rx_call_back_1(rxBuffer);
    };

    friend void ::HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan);
    friend void ::HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan);
};

} // namespace CAN
} // namespace llll

#endif // LLLL_CAN_HPP
