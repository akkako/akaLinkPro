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
#include "ws2812.h"
#include "api_param.h"

int main(void)
{
    board_init();
    WS2812_Init();
    api_param_load();

    /* USB composite device: CMSIS-DAP (enumeration) + DFU runtime interface.
     * dfu-util -e targets the DFU runtime interface and reboots to bootloader. */
    extern void chry_dap_init(uint8_t busid, uint32_t reg_base);
    board_init_usb((USB_Type *)CONFIG_HPM_USBD_BASE);
    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 2);
    chry_dap_init(0, CONFIG_HPM_USBD_BASE);

    int key_press_count = 0;

    WS2812_SetColor(0xFF / 8);

    while (1)
    {
        ewdg_refresh(HPM_EWDG0);
        /* Key detection: check every 100ms, 5 consecutive presses -> boot */
#ifdef BOARD_APP_GPIO_CTRL
        uint8_t key = gpio_read_pin(BOARD_APP_GPIO_CTRL,
                                    BOARD_APP_GPIO_INDEX,
                                    BOARD_APP_GPIO_PIN);
        if (key == BOARD_BTN_PRESSED_VALUE)
        {
            key_press_count++;
            if (key_press_count >= 30)
            {
                printf("\n[KEY] Held 3000ms, entering bootloader...\n");
                while (gpio_read_pin(BOARD_APP_GPIO_CTRL,
                                     BOARD_APP_GPIO_INDEX,
                                     BOARD_APP_GPIO_PIN) == BOARD_BTN_PRESSED_VALUE)
                {
                    board_delay_ms(10);
                    WS2812_ShowRainbow();
                }
                board_delay_ms(50);

                hpm_dfu_reboot_to_dfu();
            }
        }
        else
        {
            key_press_count = 0;
        }
#endif
        board_delay_ms(100);
    }
    return 0;
}
