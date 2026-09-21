/*
 * Copyright (c) 2021 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdio.h>
#include "board.h"
#include "hpm_debug_console.h"
#include "hpm_gpio_drv.h"
#include "hpm_soc.h"
#include "hpm_usb_drv.h"
#include "hpm_interrupt.h"
#include "usb_config.h"
#include "hpm_dfu_trigger.h"
#include "api_param.h"
#include "usb_composite.h"

int main(void)
{
    board_init();
    api_param_load();

    board_init_usb((USB_Type *)CONFIG_HPM_USBD_BASE);
    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 2);
    chry_dap_init(0, CONFIG_HPM_USBD_BASE);

    /* Bring up the UART2 <-> CDC COM port bridge. DAP_SETUP() above parks
     * PA08/PA09 as GPIO, so this must run afterwards to mux them to UART2. */
    uartx_preinit();

    while (1)
    {
        chry_dap_handle();
        chry_dap_usb2uart_handle();
    }
    return 0;
}
