/*
 * HPM DFU Trigger Implementation
 *
 * Uses retention registers (BGPR/PDGO) to persist a DFU-entry request
 * across reset. Call hpm_dfu_reboot_to_dfu() from the APP to request
 * DFU mode on next boot.
 */
#include "hpm_dfu_trigger.h"

#include <stdio.h>
#include "board.h"
#include "hpm_common.h"
#include "hpm_soc.h"
#include "hpm_ppor_drv.h"
#ifdef HPM_BCFG_BASE
#include "hpm_bgpr_drv.h"
#endif
#ifdef HPM_PDGO_BASE
#include "hpm_pdgo_drv.h"
#endif

#define DFU_TRIGGER_MAGIC      (0x55464455UL)
#define DFU_TRIGGER_BGPR_INDEX (0U)


/* ---------- DFU runtime class handler ----------
 * Implements the minimal DFU runtime subset (DFU 1.1) so that dfu-util can
 * enumerate the interface and trigger a reboot into the DFU bootloader via
 * DFU_DETACH. The actual firmware transfer happens in the bootloader, not here.
 */
enum
{
    DFU_DETACH = 0,
    DFU_DNLOAD = 1,
    DFU_UPLOAD = 2,
    DFU_GETSTATUS = 3,
    DFU_CLRSTATUS = 4,
    DFU_GETSTATE = 5,
    DFU_ABORT = 6,
};

int dfu_runtime_handler(uint8_t busid, struct usb_setup_packet *setup,
                               uint8_t **data, uint32_t *len)
{
    (void)busid;
    switch (setup->bRequest)
    {
    case DFU_DETACH:
        /* bitWillDetach is set: reboot to DFU bootloader immediately.
         * hpm_dfu_reboot_to_dfu() never returns. */
        hpm_dfu_reboot_to_dfu();
        return 0;
    case DFU_GETSTATUS:
    {
        static uint8_t status[6] = {0, 0, 0, 0, 0, 0}; /* bStatus=OK, bwPollTimeout=0, bState=appIDLE, iString=0 */
        *data = status;
        *len = sizeof(status);
        return 0;
    }
    case DFU_GETSTATE:
    {
        static uint8_t state = 0; /* appIDLE */
        *data = &state;
        *len = 1;
        return 0;
    }
    case DFU_CLRSTATUS:
    case DFU_ABORT:
    case DFU_DNLOAD:
    case DFU_UPLOAD:
        /* Runtime mode: no operation. Acknowledge and stay in appIDLE. */
        *len = 0;
        return 0;
    default:
        return -1;
    }
}


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
    while (1) {
    }
}
