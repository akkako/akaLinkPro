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
#include "JTAG_DP.h"
#include "hpm_common.h"

// JTAG Macros

#define PIN_TCK_SET PIN_SWCLK_TCK_SET
#define PIN_TCK_CLR PIN_SWCLK_TCK_CLR
#define PIN_TMS_SET PIN_SWDIO_TMS_SET
#define PIN_TMS_CLR PIN_SWDIO_TMS_CLR

#define PIN_DELAY() PIN_DELAY_SLOW(DAP_Data.clock_delay)

static inline void JTAG_CYCLE_TCK(void)
{
    PIN_TCK_CLR();
    PIN_DELAY();
    PIN_TCK_SET();
    PIN_DELAY();
}

static inline void JTAG_CYCLE_TDI(uint32_t tdi)
{
    PIN_TDI_OUT(tdi);
    PIN_TCK_CLR();
    PIN_DELAY();
    PIN_TCK_SET();
    PIN_DELAY();
}

static inline uint32_t JTAG_CYCLE_TDO(void)
{
    uint32_t tdo;
    PIN_TCK_CLR();
    PIN_DELAY();
    PIN_DELAY();
    tdo = PIN_TDO_IN();
    PIN_TCK_SET();
    PIN_DELAY();
    return tdo;
}

static inline uint32_t JTAG_CYCLE_TDIO(uint32_t tdi)
{
    uint32_t tdo;
    PIN_TDI_OUT(tdi);
    PIN_TCK_CLR();
    PIN_DELAY();
    PIN_DELAY();
    tdo = PIN_TDO_IN();
    PIN_TCK_SET();
    PIN_DELAY();
    return tdo;
}

#if (DAP_JTAG != 0)

// Generate JTAG Sequence
//   info:   sequence information
//   tdi:    pointer to TDI generated data
//   tdo:    pointer to TDO captured data
//   return: none
ATTR_RAMFUNC void JTAG_Sequence_Slow(uint32_t delay, uint32_t info, const uint8_t *tdi, uint8_t *tdo)
{
    uint32_t i_val;
    uint32_t o_val;
    uint32_t bit;
    uint32_t n, k;

    PIN_SWDIO_TMS_OUT(info >> 6);

    if (info == 0x1)
    {
        PIN_TCK_CLR();
        PIN_DELAY();
        PIN_TCK_SET();
        return;
    }

    n = info & JTAG_SEQUENCE_TCK;

    if (n == 0U)
    {
        n = 64U;
    }

    while (n)
    {
        i_val = *tdi++;
        o_val = 0U;
        for (k = 8U; k && n; k--, n--)
        {
            bit = JTAG_CYCLE_TDIO(i_val);
            i_val >>= 1;
            o_val >>= 1;
            o_val |= bit << 7;
        }
        o_val >>= k;
        if (info & JTAG_SEQUENCE_TDO)
        {
            *tdo++ = (uint8_t)o_val;
        }
    }
}

// JTAG Set IR
//   ir:     IR value
//   return: none
ATTR_RAMFUNC void JTAG_IR_Slow(uint32_t ir)
{
    uint32_t n;

    PIN_TMS_SET();
    JTAG_CYCLE_TCK(); /* Select-DR-Scan */
    JTAG_CYCLE_TCK(); /* Select-IR-Scan */
    PIN_TMS_CLR();
    JTAG_CYCLE_TCK(); /* Capture-IR */
    JTAG_CYCLE_TCK(); /* Shift-IR */

    PIN_TDI_OUT(1U);
    for (n = DAP_Data.jtag_dev.ir_before[DAP_Data.jtag_dev.index]; n; n--)
    {
        JTAG_CYCLE_TCK(); /* Bypass before data */
    }
    for (n = DAP_Data.jtag_dev.ir_length[DAP_Data.jtag_dev.index] - 1U; n; n--)
    {
        JTAG_CYCLE_TDI(ir); /* Set IR bits (except last) */
        ir >>= 1;
    }
    n = DAP_Data.jtag_dev.ir_after[DAP_Data.jtag_dev.index];
    if (n)
    {
        JTAG_CYCLE_TDI(ir); /* Set last IR bit */
        PIN_TDI_OUT(1U);
        for (--n; n; n--)
        {
            JTAG_CYCLE_TCK(); /* Bypass after data */
        }
        PIN_TMS_SET();
        JTAG_CYCLE_TCK(); /* Bypass & Exit1-IR */
    }
    else
    {
        PIN_TMS_SET();
        JTAG_CYCLE_TDI(ir); /* Set last IR bit & Exit1-IR */
    }

    JTAG_CYCLE_TCK(); /* Update-IR */
    PIN_TMS_CLR();
    JTAG_CYCLE_TCK(); /* Idle */
    PIN_TDI_OUT(1U);
}

// JTAG Read IDCODE register
//   return: value read
ATTR_RAMFUNC uint32_t JTAG_ReadIDCode_Slow(void)
{
    uint32_t bit;
    uint32_t val;
    uint32_t n;

    PIN_TMS_SET();
    JTAG_CYCLE_TCK(); /* Select-DR-Scan */
    PIN_TMS_CLR();
    JTAG_CYCLE_TCK(); /* Capture-DR */
    JTAG_CYCLE_TCK(); /* Shift-DR */

    for (n = DAP_Data.jtag_dev.index; n; n--)
    {
        JTAG_CYCLE_TCK(); /* Bypass before data */
    }

    val = 0U;
    for (n = 31U; n; n--)
    {
        bit = JTAG_CYCLE_TDO(); /* Get D0..D30 */
        val |= bit << 31;
        val >>= 1;
    }
    PIN_TMS_SET();
    bit = JTAG_CYCLE_TDO(); /* Get D31 & Exit1-DR */
    val |= bit << 31;

    JTAG_CYCLE_TCK(); /* Update-DR */
    PIN_TMS_CLR();
    JTAG_CYCLE_TCK(); /* Idle */

    return (val);
}

// JTAG Write ABORT register
//   data:   value to write
//   return: none
ATTR_RAMFUNC void JTAG_WriteAbort_Slow(uint32_t data)
{
    uint32_t n;

    PIN_TMS_SET();
    JTAG_CYCLE_TCK(); /* Select-DR-Scan */
    PIN_TMS_CLR();
    JTAG_CYCLE_TCK(); /* Capture-DR */
    JTAG_CYCLE_TCK(); /* Shift-DR */

    for (n = DAP_Data.jtag_dev.index; n; n--)
    {
        JTAG_CYCLE_TCK(); /* Bypass before data */
    }

    PIN_TDI_OUT(0U);
    JTAG_CYCLE_TCK(); /* Set RnW=0 (Write) */
    JTAG_CYCLE_TCK(); /* Set A2=0 */
    JTAG_CYCLE_TCK(); /* Set A3=0 */

    for (n = 31U; n; n--)
    {
        JTAG_CYCLE_TDI(data); /* Set D0..D30 */
        data >>= 1;
    }
    n = DAP_Data.jtag_dev.count - DAP_Data.jtag_dev.index - 1U;
    if (n)
    {
        JTAG_CYCLE_TDI(data); /* Set D31 */
        for (--n; n; n--)
        {
            JTAG_CYCLE_TCK(); /* Bypass after data */
        }
        PIN_TMS_SET();
        JTAG_CYCLE_TCK(); /* Bypass & Exit1-DR */
    }
    else
    {
        PIN_TMS_SET();
        JTAG_CYCLE_TDI(data); /* Set D31 & Exit1-DR */
    }

    JTAG_CYCLE_TCK(); /* Update-DR */
    PIN_TMS_CLR();
    JTAG_CYCLE_TCK(); /* Idle */
    PIN_TDI_OUT(1U);
}

// JTAG Read
//   request: A[3:2] RnW APnDP
//   data:    DATA[31:0]
//   return:  ACK[2:0]
ATTR_RAMFUNC uint8_t JTAG_Read_Slow(uint32_t request, uint32_t *data)
{
    uint32_t ack;
    uint32_t bit;
    uint32_t val;
    uint32_t n;

    PIN_TMS_SET();
    JTAG_CYCLE_TCK(); /* Select-DR-Scan */
    PIN_TMS_CLR();
    JTAG_CYCLE_TCK(); /* Capture-DR */
    JTAG_CYCLE_TCK(); /* Shift-DR */

    for (n = DAP_Data.jtag_dev.index; n; n--)
    {
        JTAG_CYCLE_TCK(); /* Bypass before data */
    }

    bit = JTAG_CYCLE_TDIO(request >> 1); /* Set RnW, Get ACK.0 */
    ack = bit << 1;
    bit = JTAG_CYCLE_TDIO(request >> 2); /* Set A2,  Get ACK.1 */
    ack |= bit << 0;
    bit = JTAG_CYCLE_TDIO(request >> 3); /* Set A3,  Get ACK.2 */
    ack |= bit << 2;

    if (ack != DAP_TRANSFER_OK)
    {
        /* Exit on error */
        PIN_TMS_SET();
        JTAG_CYCLE_TCK(); /* Exit1-DR */
        goto exit;
    }

    /* Read Transfer */
    val = 0U;
    for (n = 31U; n; n--)
    {
        bit = JTAG_CYCLE_TDO(); /* Get D0..D30 */
        val |= bit << 31;
        val >>= 1;
    }
    n = DAP_Data.jtag_dev.count - DAP_Data.jtag_dev.index - 1U;
    if (n)
    {
        bit = JTAG_CYCLE_TDO(); /* Get D31 */
        for (--n; n; n--)
        {
            JTAG_CYCLE_TCK(); /* Bypass after data */
        }
        PIN_TMS_SET();
        JTAG_CYCLE_TCK(); /* Bypass & Exit1-DR */
    }
    else
    {
        PIN_TMS_SET();
        bit = JTAG_CYCLE_TDO(); /* Get D31 & Exit1-DR */
    }
    val |= bit << 31;
    if (data)
    {
        *data = val;
    }

exit:
    JTAG_CYCLE_TCK(); /* Update-DR */
    PIN_TMS_CLR();
    JTAG_CYCLE_TCK(); /* Idle */
    PIN_TDI_OUT(1U);

    // /* Idle cycles */
    // n = DAP_Data.transfer.idle_cycles;
    // while (n--)
    // {
    //     JTAG_CYCLE_TCK(); /* Idle */
    // }

    return ((uint8_t)ack);
}

// JTAG Write
//   request: A[3:2] RnW APnDP
//   data:    DATA[31:0]
//   return:  ACK[2:0]
ATTR_RAMFUNC uint8_t JTAG_Write_Slow(uint32_t request, uint32_t *data)
{
    uint32_t ack;
    uint32_t bit;
    uint32_t val;
    uint32_t n;

    PIN_TMS_SET();
    JTAG_CYCLE_TCK(); /* Select-DR-Scan */
    PIN_TMS_CLR();
    JTAG_CYCLE_TCK(); /* Capture-DR */
    JTAG_CYCLE_TCK(); /* Shift-DR */

    for (n = DAP_Data.jtag_dev.index; n; n--)
    {
        JTAG_CYCLE_TCK(); /* Bypass before data */
    }

    bit = JTAG_CYCLE_TDIO(request >> 1); /* Set RnW, Get ACK.0 */
    ack = bit << 1;
    bit = JTAG_CYCLE_TDIO(request >> 2); /* Set A2,  Get ACK.1 */
    ack |= bit << 0;
    bit = JTAG_CYCLE_TDIO(request >> 3); /* Set A3,  Get ACK.2 */
    ack |= bit << 2;

    if (ack != DAP_TRANSFER_OK)
    {
        /* Exit on error */
        PIN_TMS_SET();
        JTAG_CYCLE_TCK(); /* Exit1-DR */
        goto exit;
    }

    /* Write Transfer */
    val = *data;
    for (n = 31U; n; n--)
    {
        JTAG_CYCLE_TDI(val); /* Set D0..D30 */
        val >>= 1;
    }
    n = DAP_Data.jtag_dev.count - DAP_Data.jtag_dev.index - 1U;
    if (n)
    {
        JTAG_CYCLE_TDI(val); /* Set D31 */
        for (--n; n; n--)
        {
            JTAG_CYCLE_TCK(); /* Bypass after data */
        }
        PIN_TMS_SET();
        JTAG_CYCLE_TCK(); /* Bypass & Exit1-DR */
    }
    else
    {
        PIN_TMS_SET();
        JTAG_CYCLE_TDI(val); /* Set D31 & Exit1-DR */
    }

exit:
    JTAG_CYCLE_TCK(); /* Update-DR */
    PIN_TMS_CLR();
    JTAG_CYCLE_TCK(); /* Idle */
    PIN_TDI_OUT(1U);

    // /* Idle cycles */
    // n = DAP_Data.transfer.idle_cycles;
    // while (n--)
    // {
    //     JTAG_CYCLE_TCK(); /* Idle */
    // }

    return ((uint8_t)ack);
}

#endif /* (DAP_JTAG != 0) */
