/**
 * @file    llll_can.cpp
 * @author  llll
 * @date    2026-11-07
 * @brief   stm32F103CxT6 CAN通信 工具文件
 */

#include "llll_can.hpp"

using namespace llll::CAN;

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    for (uint8_t i = 0; i < Driver::driverCount; i++)
    {
        if (hcan == Driver::drivers[i]->can_handler)
        {
            HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &Driver::drivers[i]->rxBuffer.Header,
                                 Driver::drivers[i]->rxBuffer.Data);
            Driver::drivers[i]->OnReceive0(Driver::drivers[i]->rxBuffer);
        }
    }
}
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    for (uint8_t i = 0; i < Driver::driverCount; i++)
    {
        if (hcan == Driver::drivers[i]->can_handler)
        {
            HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &Driver::drivers[i]->rxBuffer.Header,
                                 Driver::drivers[i]->rxBuffer.Data);
            Driver::drivers[i]->OnReceive1(Driver::drivers[i]->rxBuffer);
        }
    }
}

HAL_StatusTypeDef llll::CAN::Driver::Send_Frame(Driver::TxBuffer tx_buffer)
{
    return HAL_CAN_AddTxMessage(can_handler, &tx_buffer.Header, tx_buffer.Data, &tx_buffer.UsedMailbox);
}

HAL_StatusTypeDef llll::CAN::Driver::Send_Data(uint32_t ID, uint32_t Size, uint8_t *Data)
{
    Driver::TxBuffer tx_buffer{
        CAN_TxHeaderTypeDef{
            .StdId = ID,
            .ExtId = 0,
            .IDE = 0,
            .RTR = 0,
            .DLC = Size,
        },
        Data,
    };
    return Send_Frame(tx_buffer);
}

void llll::CAN::Driver::Add_Filter_32_Mask(Driver::FilterID_32_Mask ID, Driver::FilterID_32_Mask MaskID, uint32_t Bank,
                                           FIFO_Channel fifo_c)
{
    uint32_t id_value =
        ((ID.STID & 0x7FFU) << 21) | ((ID.EXID & 0x3FFFFU) << 3) | ((ID.IDE & 0x1U) << 2) | ((ID.RTR & 0x1U) << 1);

    uint32_t mask_value = ((MaskID.STID & 0x7FFU) << 21) | ((MaskID.EXID & 0x3FFFFU) << 3) |
                          ((MaskID.IDE & 0x1U) << 2) | ((MaskID.RTR & 0x1U) << 1);
    CAN_FilterTypeDef can_filter_init_structure;

    can_filter_init_structure.FilterIdHigh = id_value >> 16;
    can_filter_init_structure.FilterIdLow = id_value & 0xFFFF;

    can_filter_init_structure.FilterMaskIdHigh = mask_value >> 16;
    can_filter_init_structure.FilterMaskIdLow = mask_value & 0xFFFF;

    can_filter_init_structure.FilterBank = Bank;
    can_filter_init_structure.FilterMode = CAN_FILTERMODE_IDMASK;
    can_filter_init_structure.FilterScale = CAN_FILTERSCALE_32BIT;

    can_filter_init_structure.FilterActivation = ENABLE;
    can_filter_init_structure.SlaveStartFilterBank = 14;
    can_filter_init_structure.FilterFIFOAssignment = static_cast<uint32_t>(fifo_c);
    HAL_CAN_ConfigFilter(can_handler, &can_filter_init_structure);
}
