/*
 * Copyright (c) 2023 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef HPM_PINMUX_H
#define HPM_PINMUX_H

#ifdef __cplusplus
extern "C"
{
#endif

    void init_py_pins_as_pgpio(void);
    void init_gpio_swj_pins(void);
    void init_uart_pins(UART_Type *ptr);
    void init_uart_pin_as_gpio(UART_Type *ptr);
    void init_gpio_pins(void);
    void init_butn_pins(void);
    void init_usb_pins(USB_Type *ptr);
    void init_uart_break_signal_pin(void);

#ifdef __cplusplus
}
#endif
#endif /* HPM_PINMUX_H */
