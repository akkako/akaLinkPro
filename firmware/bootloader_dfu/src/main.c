/*
 * DFU Bootloader Main
 */
#include "board.h"
#include "boot_log.h"
#include "hpm_dfu_trigger.h"
#include "boot_port_board.h"

int main(void)
{
    boot_port_board_init();

    BOOT_PRINTF("\r\n\r\n");
    BOOT_PRINTF("===========================================\r\n");
    BOOT_PRINTF("  akaLink HID MSC DFU tri-mode Bootloader\r\n");
    BOOT_PRINTF("  Build: %s %s\r\n", __DATE__, __TIME__);
    BOOT_PRINTF("===========================================\r\n\r\n");

    /* Check triggers (boot pin, BGPR magic, APP validity).
     * Jumps to APP if valid and no trigger — never returns. */
    hpm_dfu_check_bootloader_request();

    BOOT_PRINTF("[BOOT] Entering DFU mode...\r\n");

    extern void dfu_boot_init(uint8_t busid, uintptr_t reg_base);
    dfu_boot_init(0, (uintptr_t)HPM_USB0_BASE);

    while (1)
    {
        hpm_dfu_delay_ms(10);
        ewdg_refresh(HPM_EWDG0);
    }
}
