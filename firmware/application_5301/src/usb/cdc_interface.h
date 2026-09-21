/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 akaInstruments */

#ifndef __CDC_INTERFACE__
#define __CDC_INTERFACE__

#define PIN_UART_TX IOC_PAD_PA08
#define PIN_UART_RX IOC_PAD_PA09

#ifdef __cplusplus
extern "C"
{
#endif

    void uartx_io_init(void);

    void uartx_preinit(void);

    /* PA08/PA09 -> UART2 (COM mode): DAP in SWD mode, disconnected or idle. */
    void uartx_enter_com_mode(void);

    /* PA08/PA09 -> JTAG TDI/TDO FGPIO: DAP in JTAG mode. */
    void uartx_enter_jtag_mode(void);

    void usb2uart_handler(void);

#ifdef __cplusplus
}
#endif

#endif //__CDC_INTERFACE__
