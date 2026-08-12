/*
 * Copyright (c) 2013-2022 ARM Limited. All rights reserved.
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
 * $Date:        5. December 2022
 * $Revision:    V2.1.2
 *
 * Project:      CMSIS-DAP Include
 * Title:        DAP.h Definitions
 *
 *---------------------------------------------------------------------------*/

#ifndef __DAP_H__
#define __DAP_H__


// DAP Firmware Version
#ifdef  DAP_FW_V1
#define DAP_FW_VER                      "1.3.0"
#else
#define DAP_FW_VER                      "2.1.2"
#endif

// DAP Command IDs
#define ID_DAP_Info                     0x00U
#define ID_DAP_HostStatus               0x01U
#define ID_DAP_Connect                  0x02U
#define ID_DAP_Disconnect               0x03U
#define ID_DAP_TransferConfigure        0x04U
#define ID_DAP_Transfer                 0x05U
#define ID_DAP_TransferBlock            0x06U
#define ID_DAP_TransferAbort            0x07U
#define ID_DAP_WriteABORT               0x08U
#define ID_DAP_Delay                    0x09U
#define ID_DAP_ResetTarget              0x0AU
#define ID_DAP_SWJ_Pins                 0x10U
#define ID_DAP_SWJ_Clock                0x11U
#define ID_DAP_SWJ_Sequence             0x12U
#define ID_DAP_SWD_Configure            0x13U
#define ID_DAP_SWD_Sequence             0x1DU
#define ID_DAP_JTAG_Sequence            0x14U
#define ID_DAP_JTAG_Configure           0x15U
#define ID_DAP_JTAG_IDCODE              0x16U
#define ID_DAP_SWO_Transport            0x17U
#define ID_DAP_SWO_Mode                 0x18U
#define ID_DAP_SWO_Baudrate             0x19U
#define ID_DAP_SWO_Control              0x1AU
#define ID_DAP_SWO_Status               0x1BU
#define ID_DAP_SWO_ExtendedStatus       0x1EU
#define ID_DAP_SWO_Data                 0x1CU

#define ID_DAP_QueueCommands            0x7EU
#define ID_DAP_ExecuteCommands          0x7FU


#define ID_DAP_Vendor0                  0x80U
#define ID_DAP_Vendor31                 0x9FU

#define ID_DAP_Invalid                  0xFFU

// DAP Status Code
#define DAP_OK                          0U
#define DAP_ERROR                       0xFFU

// DAP ID
#define DAP_ID_VENDOR                   1U
#define DAP_ID_PRODUCT                  2U
#define DAP_ID_SER_NUM                  3U
#define DAP_ID_DAP_FW_VER               4U
#define DAP_ID_DEVICE_VENDOR            5U
#define DAP_ID_DEVICE_NAME              6U
#define DAP_ID_BOARD_VENDOR             7U
#define DAP_ID_BOARD_NAME               8U
#define DAP_ID_PRODUCT_FW_VER           9U
#define DAP_ID_CAPABILITIES             0xF0U
#define DAP_ID_TIMESTAMP_CLOCK          0xF1U
#define DAP_ID_UART_RX_BUFFER_SIZE      0xFBU
#define DAP_ID_UART_TX_BUFFER_SIZE      0xFCU
#define DAP_ID_SWO_BUFFER_SIZE          0xFDU
#define DAP_ID_PACKET_COUNT             0xFEU
#define DAP_ID_PACKET_SIZE              0xFFU

// DAP Host Status
#define DAP_DEBUGGER_CONNECTED          0U
#define DAP_TARGET_RUNNING              1U

// DAP Port
#define DAP_PORT_AUTODETECT             0U      // Autodetect Port
#define DAP_PORT_DISABLED               0U      // Port Disabled (I/O pins in High-Z)
#define DAP_PORT_SWD                    1U      // SWD Port (SWCLK, SWDIO) + nRESET
#define DAP_PORT_JTAG                   2U      // JTAG Port (TCK, TMS, TDI, TDO, nTRST) + nRESET

// DAP SWJ Pins
#define DAP_SWJ_SWCLK_TCK               0       // SWCLK/TCK
#define DAP_SWJ_SWDIO_TMS               1       // SWDIO/TMS
#define DAP_SWJ_TDI                     2       // TDI
#define DAP_SWJ_TDO                     3       // TDO
#define DAP_SWJ_nTRST                   5       // nTRST
#define DAP_SWJ_nRESET                  7       // nRESET

// DAP Transfer Request
#define DAP_TRANSFER_APnDP              (1U<<0)
#define DAP_TRANSFER_RnW                (1U<<1)
#define DAP_TRANSFER_A2                 (1U<<2)
#define DAP_TRANSFER_A3                 (1U<<3)
#define DAP_TRANSFER_MATCH_VALUE        (1U<<4)
#define DAP_TRANSFER_MATCH_MASK         (1U<<5)
#define DAP_TRANSFER_TIMESTAMP          (1U<<7)

// DAP Transfer Response
#define DAP_TRANSFER_OK                 (1U<<0)
#define DAP_TRANSFER_WAIT               (1U<<1)
#define DAP_TRANSFER_FAULT              (1U<<2)
#define DAP_TRANSFER_ERROR              (1U<<3)
#define DAP_TRANSFER_MISMATCH           (1U<<4)

// DAP SWO Trace Mode
#define DAP_SWO_OFF                     0U
#define DAP_SWO_UART                    1U
#define DAP_SWO_MANCHESTER              2U

// DAP SWO Trace Status
#define DAP_SWO_CAPTURE_ACTIVE          (1U<<0)
#define DAP_SWO_CAPTURE_PAUSED          (1U<<1)
#define DAP_SWO_STREAM_ERROR            (1U<<6)
#define DAP_SWO_BUFFER_OVERRUN          (1U<<7)

// Debug Port Register Addresses
#define DP_IDCODE                       0x00U   // IDCODE Register (SW Read only)
#define DP_ABORT                        0x00U   // Abort Register (SW Write only)
#define DP_CTRL_STAT                    0x04U   // Control & Status
#define DP_WCR                          0x04U   // Wire Control Register (SW Only)
#define DP_SELECT                       0x08U   // Select Register (JTAG R/W & SW W)
#define DP_RESEND                       0x08U   // Resend (SW Read Only)
#define DP_RDBUFF                       0x0CU   // Read Buffer (Read Only)

// JTAG IR Codes
#define JTAG_ABORT                      0x08U
#define JTAG_DPACC                      0x0AU
#define JTAG_APACC                      0x0BU
#define JTAG_IDCODE                     0x0EU
#define JTAG_BYPASS                     0x0FU

// JTAG Sequence Info
#define JTAG_SEQUENCE_TCK               0x3FU   // TCK count
#define JTAG_SEQUENCE_TMS               0x40U   // TMS value
#define JTAG_SEQUENCE_TDO               0x80U   // TDO capture

// SWD Sequence Info
#define SWD_SEQUENCE_CLK                0x3FU   // SWCLK count
#define SWD_SEQUENCE_DIN                0x80U   // SWDIO capture


#include <stddef.h>
#include <stdint.h>

// DAP Data structure
typedef struct {
  uint8_t     debug_port;                       // Debug Port
  uint8_t     fast_clock;                       // Fast Clock Flag
  uint8_t     transfer_abort;                    // Transfer Abort Flag
  uint8_t     padding;
  uint32_t   clock_delay;                       // Clock Delay
  uint32_t     timestamp;                       // Last captured Timestamp
  struct {                                      // Transfer Configuration
    uint8_t   idle_cycles;                      // Idle cycles after transfer
    uint8_t    padding[3];
    uint16_t  retry_count;                      // Number of retries after WAIT response
    uint16_t  match_retry;                      // Number of retries if read value does not match
    uint32_t  match_mask;                       // Match Mask
  } transfer;
#if (DAP_SWD != 0)
  struct {                                      // SWD Configuration
    uint8_t    turnaround;                      // Turnaround period
    uint8_t    data_phase;                      // Always generate Data Phase
  } swd_conf;
#endif
#if (DAP_JTAG != 0)
  struct {                                      // JTAG Device Chain
    uint8_t   count;                            // Number of devices
    uint8_t   index;                            // Device index (device at TDO has index 0)
#if (DAP_JTAG_DEV_CNT != 0)
    uint8_t   ir_length[DAP_JTAG_DEV_CNT];      // IR Length in bits
    uint16_t  ir_before[DAP_JTAG_DEV_CNT];      // Bits before IR
    uint16_t  ir_after [DAP_JTAG_DEV_CNT];      // Bits after IR
#endif
  } jtag_dev;
#endif
} DAP_Data_t;

extern DAP_Data_t DAP_Data;            // DAP Data

#ifdef  __cplusplus
extern "C"
{
#endif

// Functions
extern void     SWJ_Sequence    (uint32_t count, const uint8_t *data);
extern void     SWD_Sequence    (uint32_t info,  const uint8_t *swdo, uint8_t *swdi);
extern void     JTAG_Sequence   (uint32_t info,  const uint8_t *tdi,  uint8_t *tdo);
extern void     JTAG_IR         (uint32_t ir);
extern uint32_t JTAG_ReadIDCode (void);
extern void     JTAG_WriteAbort (uint32_t data);
extern uint8_t  JTAG_Transfer   (uint32_t request, uint32_t *data);
extern uint8_t  SWD_Transfer    (uint32_t request, uint32_t *data);

extern uint32_t SWO_Transport      (const uint8_t *request, uint8_t *response);
extern uint32_t SWO_Mode           (const uint8_t *request, uint8_t *response);
extern uint32_t SWO_Baudrate       (const uint8_t *request, uint8_t *response);
extern uint32_t SWO_Control        (const uint8_t *request, uint8_t *response);
extern uint32_t SWO_Status                                 (uint8_t *response);
extern uint32_t SWO_ExtendedStatus (const uint8_t *request, uint8_t *response);
extern uint32_t SWO_Data           (const uint8_t *request, uint8_t *response);

extern void     SWO_QueueTransfer    (uint8_t *buf, uint32_t num);
extern void     SWO_AbortTransfer    (void);
extern void     SWO_TransferComplete (void);

extern uint32_t SWO_Mode_UART     (uint32_t enable);
extern uint32_t SWO_Baudrate_UART (uint32_t baudrate);
extern uint32_t SWO_Control_UART  (uint32_t active);
extern void     SWO_Capture_UART  (uint8_t *buf, uint32_t num);
extern uint32_t SWO_GetCount_UART (void);

extern uint32_t SWO_Mode_Manchester     (uint32_t enable);
extern uint32_t SWO_Baudrate_Manchester (uint32_t baudrate);
extern uint32_t SWO_Control_Manchester  (uint32_t active);
extern void     SWO_Capture_Manchester  (uint8_t *buf, uint32_t num);
extern uint32_t SWO_GetCount_Manchester (void);

extern uint32_t DAP_ProcessVendorCommand (const uint8_t *request, uint8_t *response);
extern uint32_t DAP_ProcessCommand       (const uint8_t *request, uint8_t *response);
extern uint32_t DAP_ExecuteCommand       (const uint8_t *request, uint8_t *response);

extern void     DAP_Setup (void);

// Configurable delay for clock generation
#define DELAY_SLOW_CYCLES       3U      // Number of cycles for one iteration

__STATIC_FORCEINLINE void PIN_DELAY_SLOW (uint32_t delay) {
  uint32_t count = delay;
  while (--count)
  {
    NOP();
  }
}

// Fixed delay for fast clock generation
#define DELAY_FAST_CYCLES       3U      // Number of cycles: 0..3

__STATIC_FORCEINLINE void PIN_DELAY_FAST (void) {
#if (DELAY_FAST_CYCLES >= 1U)
  NOP();
#endif
#if (DELAY_FAST_CYCLES >= 2U)
  NOP();
#endif
#if (DELAY_FAST_CYCLES >= 3U)
  NOP();
#endif
}

#ifdef  __cplusplus
}
#endif


#endif  /* __DAP_H__ */
