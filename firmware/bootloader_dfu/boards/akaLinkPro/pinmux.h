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
    void init_dfu_pins(void);
    void init_usb_pins(USB_Type *ptr);
    void init_led_pins(void);
    void init_power_pins(void);

#ifdef __cplusplus
}
#endif
#endif /* HPM_PINMUX_H */
