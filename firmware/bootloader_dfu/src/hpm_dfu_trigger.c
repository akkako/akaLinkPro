/*
 * HPM DFU Boot Port — trigger, jump, and bootloader entry logic.
 * Adapted from hpm_sdk samples/cherryusb/device/dfu/common/hpm_dfu_trigger.c
 */
#include "hpm_dfu_trigger.h"

#include <stdio.h>
#include "board.h"
#include "boot_log.h"
#include "hpm_common.h"
#include "hpm_soc.h"
#include "hpm_l1c_drv.h"
#include "hpm_ppor_drv.h"
#include "ws2812.h"
#ifdef HPM_BCFG_BASE
#include "hpm_bgpr_drv.h"
#endif
#ifdef HPM_PDGO_BASE
#include "hpm_pdgo_drv.h"
#endif

#define DFU_TRIGGER_MAGIC      (0x55464455UL)
#define DFU_TRIGGER_BGPR_INDEX (0U)

/*---------------------------------------------------------------
 * Retention register trigger (BGPR / PDGO)
 *-------------------------------------------------------------*/
bool hpm_dfu_check_and_clear_trigger(void)
{
    bool triggered = false;
#ifdef HPM_BCFG_BASE
    uint32_t val = 0;
    if (bgpr_read32(BOARD_BGPR, DFU_TRIGGER_BGPR_INDEX, &val) == status_success) {
        if (val == DFU_TRIGGER_MAGIC) {
            (void)bgpr_write32(BOARD_BGPR, DFU_TRIGGER_BGPR_INDEX, 0);
            triggered = true;
        }
    }
#endif
#ifdef HPM_PDGO_BASE
    if (pdgo_is_retention_mode_enabled(HPM_PDGO)) {
        if (pdgo_read_gpr(HPM_PDGO, DFU_TRIGGER_BGPR_INDEX) == DFU_TRIGGER_MAGIC) {
            pdgo_write_gpr(HPM_PDGO, DFU_TRIGGER_BGPR_INDEX, 0);
            triggered = true;
        }
    }
#endif
    return triggered;
}

void hpm_dfu_reboot_to_dfu(void)
{
#ifdef HPM_BCFG_BASE
    (void)bgpr_write32(BOARD_BGPR, DFU_TRIGGER_BGPR_INDEX, DFU_TRIGGER_MAGIC);
#endif
#ifdef HPM_PDGO_BASE
    if (!pdgo_is_retention_mode_enabled(HPM_PDGO)) {
        pdgo_enable_retention_mode(HPM_PDGO);
    }
    pdgo_write_gpr(HPM_PDGO, DFU_TRIGGER_BGPR_INDEX, DFU_TRIGGER_MAGIC);
#endif
    printf("dfu trigger received, reboot to DFU bootloader...\r\n");
    ppor_reset_mask_set_source_enable(HPM_PPOR, ppor_reset_software);
    ppor_sw_reset(HPM_PPOR, 24);
    while (1) {}
}

/*---------------------------------------------------------------
 * Jump to APP (skip 4-byte DFU signature)
 *-------------------------------------------------------------*/
void hpm_dfu_jump_to_app(void)
{
    uint32_t entry = USBD_DFU_APP_DEFAULT_ADD + 4;

    WS2812_TurnOff();

    BOOT_PRINTF("[BOOT] Jumping to application at 0x%08lx\r\n",
                (unsigned long)USBD_DFU_APP_DEFAULT_ADD);
    disable_global_irq(CSR_MSTATUS_MIE_MASK);
    fencei();
    l1c_dc_disable();
    __asm volatile ("jr %0\n" : : "r" (entry) : );
    while (1);
}

/*---------------------------------------------------------------
 * Bootloader entry check: pin, trigger, signature.
 * Jumps to APP if valid — never returns in that case.
 *-------------------------------------------------------------*/
void hpm_dfu_check_bootloader_request(void)
{
    extern bool boot_port_board_read_bootpin(void);
    if (boot_port_board_read_bootpin()) {
        BOOT_PRINTF("[BOOT] Boot pin active, staying in bootloader\r\n");
        /* Clear any stale DFU trigger so the next reset boots APP */
        (void)hpm_dfu_check_and_clear_trigger();
        return;
    }
    if (hpm_dfu_check_and_clear_trigger()) {
        BOOT_PRINTF("[BOOT] DFU trigger from APP, staying in bootloader\r\n");
        return;
    }
    if (*(volatile uint32_t *)USBD_DFU_APP_DEFAULT_ADD == BOARD_DFU_SIGNATURE) {
        BOOT_PRINTF("[BOOT] Valid APP, jumping...\r\n");
        extern void boot_port_board_deinit(void);
        boot_port_board_deinit();
        hpm_dfu_jump_to_app();
    }
    BOOT_PRINTF("[BOOT] No valid application, staying in bootloader\r\n");
}

/*---------------------------------------------------------------
 * Millisecond delay
 *-------------------------------------------------------------*/
void hpm_dfu_delay_ms(uint32_t ms)
{
    board_delay_ms(ms);
}
