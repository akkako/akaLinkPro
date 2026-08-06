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

#include "DAP_Port.h"
#include "DAP.h"
#include <stdio.h>

// JTAG Macros

#define PIN_TCK_SET PIN_SWCLK_TCK_SET
#define PIN_TCK_CLR PIN_SWCLK_TCK_CLR
#define PIN_TMS_SET PIN_SWDIO_TMS_SET
#define PIN_TMS_CLR PIN_SWDIO_TMS_CLR

#define PIN_DELAY() PIN_DELAY_FAST()

static inline void JTAG_CYCLE_TCK(void)
{
    PIN_TCK_CLR();
    // PIN_DELAY();
    PIN_TCK_SET();
    // PIN_DELAY();
}

static inline void JTAG_CYCLE_TDI(uint32_t tdi)
{
    PIN_TDI_OUT(tdi);
    PIN_TCK_CLR();
    // PIN_DELAY();
    PIN_TCK_SET();
    // PIN_DELAY();
}

static inline uint32_t JTAG_CYCLE_TDO(void)
{
    uint32_t tdo;

    PIN_TCK_CLR();
    // PIN_DELAY();
    PIN_TCK_SET();
    tdo = PIN_TDO_IN();
    // PIN_DELAY();

    return tdo;
}

static inline uint32_t JTAG_CYCLE_TDIO(uint32_t tdi)
{
    uint32_t tdo;

    PIN_TDI_OUT(tdi);
    PIN_TCK_CLR();
    // PIN_DELAY();
    // PIN_DELAY();
    PIN_TCK_SET();
    tdo = PIN_TDO_IN();
    // PIN_DELAY();

    return tdo;
}

// Generate JTAG Sequence
//   info:   sequence information
//   tdi:    pointer to TDI generated data
//   tdo:    pointer to TDO captured data
//   return: none
void JTAG_Sequence(uint32_t info, const uint8_t *tdi, uint8_t *tdo)
{
    uint32_t n = info & JTAG_SEQUENCE_TCK;
    uint32_t i_val, o_val, bit;

    /* TMS 在整段序列中保持不变，提前设置 */
    if (info & JTAG_SEQUENCE_TMS)
    {
        PIN_TMS_SET();
    }
    else
    {
        PIN_TMS_CLR();
    }

    if (n == 1)
    {
        if (info & JTAG_SEQUENCE_TDO)
        {
            *tdo = JTAG_CYCLE_TDIO(*tdi);
        }
        else
        {
            JTAG_CYCLE_TDI(*tdi);
        }
        return;
    }

    if (n == 0U)
    {
        n = 64U;
    }

    uint32_t loop = n / 8;
    uint32_t remain = n % 8;

    if (info & JTAG_SEQUENCE_TDO)
    {
        for (uint32_t i = 0; i < loop; i++)
        {
            i_val = *tdi++;
            o_val = 0U;
            bit = JTAG_CYCLE_TDIO(i_val);
            o_val |= (bit << 0);
            i_val >>= 1;
            bit = JTAG_CYCLE_TDIO(i_val);
            o_val |= (bit << 1);
            i_val >>= 1;
            bit = JTAG_CYCLE_TDIO(i_val);
            o_val |= (bit << 2);
            i_val >>= 1;
            bit = JTAG_CYCLE_TDIO(i_val);
            o_val |= (bit << 3);
            i_val >>= 1;
            bit = JTAG_CYCLE_TDIO(i_val);
            o_val |= (bit << 4);
            i_val >>= 1;
            bit = JTAG_CYCLE_TDIO(i_val);
            o_val |= (bit << 5);
            i_val >>= 1;
            bit = JTAG_CYCLE_TDIO(i_val);
            o_val |= (bit << 6);
            i_val >>= 1;
            bit = JTAG_CYCLE_TDIO(i_val);
            o_val |= (bit << 7);
            i_val >>= 1;
            *tdo++ = (uint8_t)o_val;
        }

        if (remain)
        {
            i_val = *tdi++;
            o_val = 0U;

            if (remain > 0U)
            {
                bit = JTAG_CYCLE_TDIO(i_val);
                o_val |= (bit << 0);
                i_val >>= 1;
            }
            if (remain > 1U)
            {
                bit = JTAG_CYCLE_TDIO(i_val);
                o_val |= (bit << 1);
                i_val >>= 1;
            }
            if (remain > 2U)
            {
                bit = JTAG_CYCLE_TDIO(i_val);
                o_val |= (bit << 2);
                i_val >>= 1;
            }
            if (remain > 3U)
            {
                bit = JTAG_CYCLE_TDIO(i_val);
                o_val |= (bit << 3);
                i_val >>= 1;
            }
            if (remain > 4U)
            {
                bit = JTAG_CYCLE_TDIO(i_val);
                o_val |= (bit << 4);
                i_val >>= 1;
            }
            if (remain > 5U)
            {
                bit = JTAG_CYCLE_TDIO(i_val);
                o_val |= (bit << 5);
                i_val >>= 1;
            }
            if (remain > 6U)
            {
                bit = JTAG_CYCLE_TDIO(i_val);
                o_val |= (bit << 6);
                i_val >>= 1;
            }
            if (remain > 7U)
            {
                bit = JTAG_CYCLE_TDIO(i_val);
                o_val |= (bit << 7);
                i_val >>= 1;
            }

            *tdo++ = (uint8_t)o_val;
        }
    }
    else
    {
        for (uint32_t i = 0; i < loop; i++)
        {
            i_val = *tdi++;
            JTAG_CYCLE_TDI(i_val);
            i_val >>= 1;
            JTAG_CYCLE_TDI(i_val);
            i_val >>= 1;
            JTAG_CYCLE_TDI(i_val);
            i_val >>= 1;
            JTAG_CYCLE_TDI(i_val);
            i_val >>= 1;
            JTAG_CYCLE_TDI(i_val);
            i_val >>= 1;
            JTAG_CYCLE_TDI(i_val);
            i_val >>= 1;
            JTAG_CYCLE_TDI(i_val);
            i_val >>= 1;
            JTAG_CYCLE_TDI(i_val);
            i_val >>= 1;
        }

        if (remain)
        {
            i_val = *tdi++;

            if (remain > 0U)
            {
                JTAG_CYCLE_TDI(i_val);
                i_val >>= 1;
            }
            if (remain > 1U)
            {
                JTAG_CYCLE_TDI(i_val);
                i_val >>= 1;
            }
            if (remain > 2U)
            {
                JTAG_CYCLE_TDI(i_val);
                i_val >>= 1;
            }
            if (remain > 3U)
            {
                JTAG_CYCLE_TDI(i_val);
                i_val >>= 1;
            }
            if (remain > 4U)
            {
                JTAG_CYCLE_TDI(i_val);
                i_val >>= 1;
            }
            if (remain > 5U)
            {
                JTAG_CYCLE_TDI(i_val);
                i_val >>= 1;
            }
            if (remain > 6U)
            {
                JTAG_CYCLE_TDI(i_val);
                i_val >>= 1;
            }
            if (remain > 7U)
            {
                JTAG_CYCLE_TDI(i_val);
                i_val >>= 1;
            }
        }
    }
}

// JTAG Set IR
//   ir:     IR value
//   return: none

static void JTAG_IR_Opt(uint32_t ir)
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
uint32_t JTAG_ReadIDCode(void)
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
void JTAG_WriteAbort(uint32_t data)
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

// JTAG Set IR
//   ir:     IR value
//   return: none
void JTAG_IR(uint32_t ir)
{
    JTAG_IR_Opt(ir);
}

static uint8_t JTAG_Transfer_Read_GPIO_Fast(uint32_t request, uint32_t *data)
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

    // for (n = 31U; n; n--) {
    //     bit = JTAG_CYCLE_TDO(); /* Get D0..D30 */
    //     val |= bit << 31;
    //     val >>= 1;
    // }

    bit = JTAG_CYCLE_TDO(); /* Get D0 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D1 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D2 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D3 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D4 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D5 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D6 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D7 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D8 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D9 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D10 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D11 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D12 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D13 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D14 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D15 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D16 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D17 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D18 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D19 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D20 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D21 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D22 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D23 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D24 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D25 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D26 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D27 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D28 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D29 */
    val |= bit << 31;
    val >>= 1;

    bit = JTAG_CYCLE_TDO(); /* Get D30 */
    val |= bit << 31;
    val >>= 1;

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

    /* Capture Timestamp */
    // if (request & DAP_TRANSFER_TIMESTAMP) {
    //     DAP_Data.timestamp = TIMESTAMP_GET();
    // }

    /* Idle cycles */
    n = DAP_Data.transfer.idle_cycles;
    while (n--)
    {
        JTAG_CYCLE_TCK(); /* Idle */
    }

    return ((uint8_t)ack);
}

static uint8_t JTAG_Transfer_Write_GPIO_Fast(uint32_t request, uint32_t *data)
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
    // for (n = 31U; n; n--) {
    //     JTAG_CYCLE_TDI (val); /* Set D0..D30 */
    //     val >>= 1;
    // }

    JTAG_CYCLE_TDI(val); /* Set D0 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D1 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D2 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D3 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D4 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D5 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D6 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D7 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D8 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D9 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D10 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D11 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D12 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D13 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D14 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D15 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D16 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D17 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D18 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D19 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D20 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D21 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D22 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D23 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D24 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D25 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D26 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D27 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D28 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D29 */
    val >>= 1;

    JTAG_CYCLE_TDI(val); /* Set D30 */
    val >>= 1;

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

    /* Capture Timestamp */
    // if (request & DAP_TRANSFER_TIMESTAMP) {
    //     DAP_Data.timestamp = TIMESTAMP_GET();
    // }

    /* Idle cycles */
    n = DAP_Data.transfer.idle_cycles;
    while (n--)
    {
        JTAG_CYCLE_TCK(); /* Idle */
    }

    return ((uint8_t)ack);
}

/**
 * @brief JTAG 传输写函数
 *
 * @param request A[3:2] RnW APnDP
 * @param data DATA[31:0]
 * @return uint8_t ACK[2:0]
 */
uint8_t JTAG_Transfer_Write(uint32_t request, uint32_t *data)
{
    return JTAG_Transfer_Write_GPIO_Fast(request, data);
}

/**
 * @brief JTAG 传输读函数
 *
 * @param request A[3:2] RnW APnDP
 * @param data DATA[31:0]
 * @return uint8_t ACK[2:0]
 */
uint8_t JTAG_Transfer_Read(uint32_t request, uint32_t *data)
{
    return JTAG_Transfer_Read_GPIO_Fast(request, data);
}
