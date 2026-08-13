/*
 * Copyright (c) 2013-2017 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ----------------------------------------------------------------------
 *
 * $Date:        1. December 2017
 * $Revision:    V2.0.0
 *
 * Project:      CMSIS-DAP Source
 * Title:        SW_DP.c CMSIS-DAP SW DP I/O
 *
 *---------------------------------------------------------------------------*/

#include "DAP_config.h"
#include "DAP.h"
#include "SW_DP.h"
#include "hpm_common.h"

#if ((DAP_SWD != 0) || (DAP_JTAG != 0))
/**
 * @brief Generate SWJ Sequence
 * @param count sequence bit count
 * @param data pointer to sequence bit data
 * @return none
 */
ATTR_RAMFUNC void SWJ_Sequence(uint32_t count, const uint8_t *data)
{
    // SWJ_Sequence_GPIO_Fast(count, data);
    SWJ_Sequence_GPIO_Slow(count, data);
}
#endif

#if (DAP_SWD != 0)
/**
 * @brief Generate SWD Sequence
 * @param info sequence information
 * @param swdo pointer to SWDIO generated data
 * @param swdi pointer to SWDIO captured data
 * @return none
 */
ATTR_RAMFUNC void SWD_Sequence(uint32_t info, const uint8_t *swdo, uint8_t *swdi)
{
    // SWD_Sequence_GPIO_Fast(info, swdo, swdi);
    SWD_Sequence_GPIO_Slow(info, swdo, swdi);
}
#endif

/**
 * @brief SWD Write
 * @param request A[3:2] RnW APnDP
 * @param data DATA[31:0]
 * @return ACK[2:0]
 */
ATTR_RAMFUNC uint8_t SWD_Write(uint32_t request, uint32_t *data)
{
    uint8_t header = 0x81 | ((request & 0x0F) << 1) | (((request ^ (request >> 1) ^ (request >> 2) ^ (request >> 3)) & 1) << 5);

    uint8_t ack =  SWD_Write_GPIO_ASM_60M(header, data);
    // printf("ack = 0x%02X\n", ack);
    return ack;
    // return SWD_Write_GPIO_Slow(header, data);
}

/**
 * @brief SWD Read
 * @param request A[3:2] RnW APnDP
 * @param data DATA[31:0]
 * @return ACK[2:0]
 */
ATTR_RAMFUNC uint8_t SWD_Read(uint32_t request, uint32_t *data)
{
    uint8_t header = 0x81 | ((request & 0x0F) << 1) | (((request ^ (request >> 1) ^ (request >> 2) ^ (request >> 3)) & 1) << 5);

    // return SWD_Read_GPIO_Fast(request, data);
    return SWD_Read_GPIO_Slow(header, data);
}
