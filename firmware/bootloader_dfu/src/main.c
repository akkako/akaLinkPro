/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 akaInstruments */

/*
 * DFU Bootloader Main
 */
#include "board.h"
#include "hpm_dfu_trigger.h"
#include "hpm_ppor_drv.h"
#include "boot_port_board.h"
#include "vfat.h"

int main(void)
{
    boot_port_board_init();

    printf("\r\n\r\n");
    printf("===========================================\r\n");
    printf("  akaLink HID MSC DFU tri-mode Bootloader\r\n");
    printf("  Build: %s %s\r\n", __DATE__, __TIME__);
    printf("===========================================\r\n\r\n");

    /* Check triggers (boot pin, BGPR magic, APP validity).
     * Jumps to APP if valid and no trigger — never returns. */
    hpm_dfu_check_bootloader_request();

    printf("[BOOT] Entering DFU mode...\r\n");

    extern void dfu_boot_init(uint8_t busid, uintptr_t reg_base);
    dfu_boot_init(0, (uintptr_t)HPM_USB0_BASE);

    while (1)
    {
        /* A dragged-in firmware has been fully written: let the host settle,
         * then reboot and let the integrity check decide app vs. U-disk. */
        if (vfat_upgrade_finished())
        {
            board_led1_on();
            board_led2_on();
            hpm_dfu_delay_ms(1000);
            ppor_reset_mask_set_source_enable(HPM_PPOR, ppor_reset_software);
            ppor_sw_reset(HPM_PPOR, 24);
            while (1) {}
        }

        board_led1_on();
        board_led2_on();
        hpm_dfu_delay_ms(500);
        board_led1_off();
        board_led2_off();
        hpm_dfu_delay_ms(500);
        ewdg_refresh(HPM_EWDG0);
    }
}
