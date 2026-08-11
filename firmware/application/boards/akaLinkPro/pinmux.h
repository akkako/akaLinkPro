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
void init_uart0_pins(void);
void init_uart3_pins_as_uart(void);
void init_uart3_pin_as_gpio_low(void);
void init_gpio_pins(void);
void init_usb0_pins(void);
void init_uart_break_signal_pin(void);

void init_unused_pin_as_input(void);
void init_jtag_swd_pin(void);

#ifdef __cplusplus
}
#endif
#endif /* HPM_PINMUX_H */
