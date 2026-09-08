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
 * Title:        JTAG_DP.c CMSIS-DAP JTAG DP I/O
 *
 *---------------------------------------------------------------------------*/

#include "DAP_config.h"
#include "DAP.h"
#include "hpm_common.h"
#include "JTAG_DP.h"

#if (DAP_JTAG != 0)

// Generate JTAG Sequence
//   info:   sequence information
//   tdi:    pointer to TDI generated data
//   tdo:    pointer to TDO captured data
//   return: none
ATTR_RAMFUNC void JTAG_Sequence(uint32_t info, const uint8_t *tdi, uint8_t *tdo)
{
    // printf("JTAG Seq: 0x%02x, 0x%02x\n", info, *tdi);
    JTAG_Sequence_Slow(0, info, tdi, tdo);
}

// JTAG Read IDCODE register
//   return: value read
ATTR_RAMFUNC uint32_t JTAG_ReadIDCode(void)
{
    // printf("JTAG ReadIDCode\n");
    return JTAG_ReadIDCode_Slow();
}

// JTAG Write ABORT register
//   data:   value to write
//   return: none
ATTR_RAMFUNC void JTAG_WriteAbort(uint32_t data)
{
    // printf("JTAG WriteAbort\n");
    uint32_t bypass_before = DAP_Data.jtag_dev.index;
    uint32_t bypass_after = DAP_Data.jtag_dev.count - DAP_Data.jtag_dev.index - 1U;
    JTAG_WriteAbort_Slow(data, bypass_before, bypass_after);
}

// JTAG Set IR
//   ir:     IR value
//   return: none
ATTR_RAMFUNC void JTAG_IR(uint32_t ir)
{
    // printf("JTAG IR\n");
    JTAG_IR_Slow(ir);
}

// JTAG Write
//   request: A[3:2] RnW APnDP
//   data:    DATA[31:0]
//   return:  ACK[2:0]
ATTR_RAMFUNC uint8_t JTAG_Write(uint32_t request, uint32_t *data)
{
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(IOC_PAD_PA10), GPIO_GET_PIN_INDEX(IOC_PAD_PA10), 1);
    
    uint32_t bypass_before = DAP_Data.jtag_dev.index;
    uint32_t bypass_after = DAP_Data.jtag_dev.count - DAP_Data.jtag_dev.index - 1U;
    uint8_t ack;

    asm("fence");

    ack = JTAG_Write_GPIO_ASM_45M(request, data, bypass_before, bypass_after);
    // uint8_t ack = JTAG_Write_Slow(request, data, bypass_before, bypass_after);

    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(IOC_PAD_PA10), GPIO_GET_PIN_INDEX(IOC_PAD_PA10), 0);

    /* Capture Timestamp */
    if (request & DAP_TRANSFER_TIMESTAMP)
    {
        DAP_Data.timestamp = TIMESTAMP_GET();
    }

    return ack;
}

// JTAG Read
//   request: A[3:2] RnW APnDP
//   data:    DATA[31:0]
//   return:  ACK[2:0]
ATTR_RAMFUNC uint8_t JTAG_Read(uint32_t request, uint32_t *data)
{
    // printf("JTAG Read\n");
    /* Capture Timestamp */
    if (request & DAP_TRANSFER_TIMESTAMP)
    {
        DAP_Data.timestamp = TIMESTAMP_GET();
    }
    uint32_t bypass_before = DAP_Data.jtag_dev.index;
    uint32_t bypass_after = DAP_Data.jtag_dev.count - DAP_Data.jtag_dev.index - 1U;

    return JTAG_Read_Slow(request, data, bypass_before, bypass_after);
}

#endif /* (DAP_JTAG != 0) */
