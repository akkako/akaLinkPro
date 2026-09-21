/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 akaInstruments */

/*
 * HPM DFU Boot Port — trigger, jump, and bootloader entry logic.
 * Adapted from hpm_sdk samples/cherryusb/device/dfu/common/hpm_dfu_trigger.c
 */
#include "hpm_dfu_trigger.h"

#include <stdio.h>
#include "board.h"
#include "hpm_common.h"
#include "hpm_soc.h"
#include "hpm_l1c_drv.h"
#include "hpm_ppor_drv.h"
#ifdef HPM_BCFG_BASE
#include "hpm_bgpr_drv.h"
#endif
#ifdef HPM_PDGO_BASE
#include "hpm_pdgo_drv.h"
#endif

#define DFU_TRIGGER_MAGIC      (0x55464455UL)
#define DFU_TRIGGER_BGPR_INDEX (0U)

/* APP image header (see application_5301/Firmware_Integrity_Plan.md).
 * 0x80020000 : 4 B DFU signature (BOARD_DFU_SIGNATURE)
 * +0x04      : app code length
 * +0x08      : app code CRC32 (seed 0x0D000721)
 * +0x0C      : header format version
 * +0x100     : app code / entry point */
#define APP_HEADER_SIZE        (0x100U)
#define APP_BASE               (USBD_DFU_APP_DEFAULT_ADD)
#define APP_CODE               (APP_BASE + APP_HEADER_SIZE)
#define APP_CODE_MAX           (BOARD_FLASH_BASE_ADDRESS + BOARD_DFU_WRITABLE_SIZE - APP_CODE)
#define APP_HEADER_VERSION     (1U)
#define APP_CRC_SEED           (0x0D000721UL)

/* CRC32 with a configurable seed (matches firmware/tools/pack.py). */
static uint32_t app_crc32(uint32_t crc, const uint8_t *buf, uint32_t len)
{
    crc ^= 0xFFFFFFFFUL;
    while (len-- != 0U)
    {
        crc ^= *buf++;
        for (uint8_t i = 0; i < 8U; i++)
        {
            if ((crc & 1U) != 0U)
            {
                crc = (crc >> 1) ^ 0xEDB88320UL;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}

/* Verify the APP header (signature + length + CRC32). */
bool hpm_dfu_app_valid(void)
{
    const volatile uint32_t *hdr = (const volatile uint32_t *)APP_BASE;
    uint32_t len;
    uint32_t crc;
    uint32_t aligned_start;
    uint32_t aligned_end;

    if (hdr[0] != BOARD_DFU_SIGNATURE)
    {
        return false;
    }
    len = hdr[1];
    crc = hdr[2];
    if ((len == 0U) || (len > APP_CODE_MAX) || (hdr[3] != APP_HEADER_VERSION))
    {
        return false;
    }

    aligned_start = HPM_L1C_CACHELINE_ALIGN_DOWN(APP_CODE);
    aligned_end = HPM_L1C_CACHELINE_ALIGN_UP(APP_CODE + len);
    l1c_dc_invalidate(aligned_start, aligned_end - aligned_start);

    return (app_crc32(APP_CRC_SEED, (const uint8_t *)APP_CODE, len) == crc);
}

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
    uint32_t entry = APP_CODE;

    printf("[BOOT] Jumping to application at 0x%08lx\r\n",
                (unsigned long)APP_CODE);
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
        printf("[BOOT] Boot pin active, staying in bootloader\r\n");
        /* Clear any stale DFU trigger so the next reset boots APP */
        (void)hpm_dfu_check_and_clear_trigger();
        return;
    }
    if (hpm_dfu_check_and_clear_trigger()) {
        printf("[BOOT] DFU trigger from APP, staying in bootloader\r\n");
        return;
    }
    if (hpm_dfu_app_valid()) {
        printf("[BOOT] Valid APP (signature + length + CRC32), jumping...\r\n");
        extern void boot_port_board_deinit(void);
        boot_port_board_deinit();
        hpm_dfu_jump_to_app();
    }
    printf("[BOOT] No valid application, staying in bootloader\r\n");
}

/*---------------------------------------------------------------
 * Millisecond delay
 *-------------------------------------------------------------*/
void hpm_dfu_delay_ms(uint32_t ms)
{
    board_delay_ms(ms);
}
