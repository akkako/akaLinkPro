/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 akaInstruments */

/*
 * Bootloader DFU flash port.
 *
 * Replaces the SDK's middleware/cherryusb/port/hpmicro/hpm_dfu_port.c (which is
 * removed from the SDK library by CMakeLists.txt). Same behaviour, but every
 * erase/program/read is limited to [USBD_DFU_APP_DEFAULT_ADD, BOARD_DFU_WRITABLE_END):
 * the last two 4K flash sectors reserved for the APP parameter store can never
 * be touched by a DFU upgrade.
 */
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "hpm_common.h"
#include "hpm_soc.h"
#include "hpm_romapi.h"
#include "hpm_l1c_drv.h"
#include "hpm_ppor_drv.h"
#include "usb_dfu.h"
#include "dfu_flash_port.h"

#ifndef USBD_DFU_APP_DEFAULT_ADD
#define USBD_DFU_APP_DEFAULT_ADD 0x80020000
#endif

#ifndef DFU_XPI_NOR_BASE
#define DFU_XPI_NOR_BASE BOARD_APP_XPI_NOR_XPI_BASE
#endif
#ifndef DFU_XPI_NOR_CFG_OPT_HDR
#define DFU_XPI_NOR_CFG_OPT_HDR BOARD_APP_XPI_NOR_CFG_OPT_HDR
#endif
#ifndef DFU_XPI_NOR_CFG_OPT_OPT0
#define DFU_XPI_NOR_CFG_OPT_OPT0 BOARD_APP_XPI_NOR_CFG_OPT_OPT0
#endif
#ifndef DFU_XPI_NOR_CFG_OPT_OPT1
#define DFU_XPI_NOR_CFG_OPT_OPT1 BOARD_APP_XPI_NOR_CFG_OPT_OPT1
#endif
#ifndef DFU_XPI_FLASH_ERASE_SIZE
#define DFU_XPI_FLASH_ERASE_SIZE (16 * 1024)
#endif

/* Exclusive upper bound of every DFU access (excludes the reserved sectors). */
#define DFU_WRITABLE_END (BOARD_FLASH_BASE_ADDRESS + BOARD_DFU_WRITABLE_SIZE)

static xpi_nor_config_t s_dfu_xpi_nor_config;
static bool s_dfu_xpi_nor_initialized = false;
volatile uint32_t flash_start_address;

static void dfu_flash_init(void)
{
    xpi_nor_config_option_t option;

    if (s_dfu_xpi_nor_initialized) {
        return;
    }

    option.header.U = DFU_XPI_NOR_CFG_OPT_HDR;
    option.option0.U = DFU_XPI_NOR_CFG_OPT_OPT0;
    option.option1.U = DFU_XPI_NOR_CFG_OPT_OPT1;

    XPI_Type *base = (XPI_Type *)DFU_XPI_NOR_BASE;
    if (rom_xpi_nor_auto_config(base, &s_dfu_xpi_nor_config, &option) == status_success) {
        s_dfu_xpi_nor_initialized = true;
    }
}

static int dfu_erase_flash(uint32_t add)
{
    uint32_t sector_size;
    uint32_t addr_offset;
    uint32_t erase_start;
    hpm_stat_t status;

    if (!s_dfu_xpi_nor_initialized) {
        printf("DFU: Flash not initialized\r\n");
        return 1;
    }

    XPI_Type *base = (XPI_Type *)DFU_XPI_NOR_BASE;
    xpi_xfer_channel_t chn = xpi_xfer_channel_auto;

    rom_xpi_nor_get_property(base, &s_dfu_xpi_nor_config, xpi_nor_property_sector_size, &sector_size);

    if ((add < USBD_DFU_APP_DEFAULT_ADD) || (add >= DFU_WRITABLE_END)) {
        printf("ERROR!Address is out of range 0x%08x(0x%08x-0x%08x)\n", add,
               (unsigned int)USBD_DFU_APP_DEFAULT_ADD, (unsigned int)DFU_WRITABLE_END);
        return 1;
    }

    addr_offset = add - (uint32_t)BOARD_FLASH_BASE_ADDRESS;
    erase_start = addr_offset & (~(sector_size - 1));

    /* Refuse an erase whose 16K block would reach into the reserved tail. */
    if ((erase_start + DFU_XPI_FLASH_ERASE_SIZE) > BOARD_DFU_WRITABLE_SIZE) {
        printf("DFU: erase would touch the reserved parameter area\r\n");
        return 1;
    }

    status = rom_xpi_nor_erase(base, chn, &s_dfu_xpi_nor_config, erase_start, DFU_XPI_FLASH_ERASE_SIZE);
    if (status != status_success) {
        printf("DFU: Flash erase failed, status=%d\r\n", status);
    }
    __asm("fence.i");

    return (status == status_success) ? 0 : 1;
}

int usbd_dfu_read(uint16_t value, const uint8_t *data, uint16_t length, uint16_t *actual_length)
{
    uint32_t i = 0;

    __asm("fence.i");
    l1c_dc_invalidate_all();

    if (value == 0) {
        if (data[0] == DFU_SPECIAL_CMD_SET_ADDRESS_POINTER) {
            memcpy((uint8_t *)&flash_start_address, &data[1], 4);
            return 0;
        }
    } else if (value > 1) {
        uint32_t addr = (value - 2) * USBD_DFU_XFER_SIZE + flash_start_address;
        if ((addr < USBD_DFU_APP_DEFAULT_ADD) || ((addr + length) > DFU_WRITABLE_END)) {
            return 1;
        }
        uint8_t *p = (uint8_t *)addr;
        uint8_t *buf = (uint8_t *)data;
        for (i = 0; i < length; i++) {
            buf[i] = p[i];
        }
        *actual_length = length;
        return length;
    }

    return -1;
}

int usbd_dfu_write(uint16_t value, const uint8_t *data, uint16_t length)
{
    dfu_flash_init();

    if (!s_dfu_xpi_nor_initialized) {
        return 1;
    }

    if (value == 0) {
        if (data[0] == DFU_SPECIAL_CMD_SET_ADDRESS_POINTER) {
            memcpy((uint8_t *)&flash_start_address, &data[1], 4);
            return 0;
        } else if (data[0] == DFU_SPECIAL_CMD_ERASE) {
            memcpy((uint8_t *)&flash_start_address, &data[1], 4);
            return dfu_erase_flash(flash_start_address);
        }
    } else if (value > 1) {
        uint32_t addr = (value - 2) * USBD_DFU_XFER_SIZE + flash_start_address;
        if ((addr < USBD_DFU_APP_DEFAULT_ADD) || ((addr + length) > DFU_WRITABLE_END)) {
            return 1;
        }

        uint32_t addr_offset = addr - (uint32_t)BOARD_FLASH_BASE_ADDRESS;
        XPI_Type *base = (XPI_Type *)DFU_XPI_NOR_BASE;
        xpi_xfer_channel_t chn = xpi_xfer_channel_auto;

        disable_global_irq(CSR_MSTATUS_MIE_MASK);
        hpm_stat_t status = rom_xpi_nor_program(base, chn, &s_dfu_xpi_nor_config,
                                                (const uint32_t *)data, addr_offset, length);
        enable_global_irq(CSR_MSTATUS_MIE_MASK);
        __asm("fence.i");

        return (status == status_success) ? 0 : 1;
    }

    return -1;
}

void usbd_dfu_reset(void)
{
    ppor_sw_reset(HPM_PPOR, 24);
    while (1) {
    }
}

/* ---------------------------------------------------------------- *
 * Shared low-level access for the virtual-FAT MSC upgrade path
 * ---------------------------------------------------------------- */

hpm_stat_t boot_flash_read(uint32_t addr, void *buf, uint32_t size)
{
    uint32_t a0 = HPM_L1C_CACHELINE_ALIGN_DOWN(addr);
    uint32_t a1 = HPM_L1C_CACHELINE_ALIGN_UP(addr + size);

    l1c_dc_invalidate(a0, a1 - a0);
    memcpy(buf, (const void *)addr, size);
    return status_success;
}

hpm_stat_t boot_flash_erase(uint32_t addr, uint32_t size)
{
    dfu_flash_init();
    if (!s_dfu_xpi_nor_initialized) {
        return status_fail;
    }
    return rom_xpi_nor_erase((XPI_Type *)DFU_XPI_NOR_BASE, xpi_xfer_channel_auto,
                             &s_dfu_xpi_nor_config, addr - (uint32_t)BOARD_FLASH_BASE_ADDRESS, size);
}

hpm_stat_t boot_flash_program(uint32_t addr, const void *buf, uint32_t size)
{
    dfu_flash_init();
    if (!s_dfu_xpi_nor_initialized) {
        return status_fail;
    }
    return rom_xpi_nor_program((XPI_Type *)DFU_XPI_NOR_BASE, xpi_xfer_channel_auto,
                               &s_dfu_xpi_nor_config, (const uint32_t *)buf,
                               addr - (uint32_t)BOARD_FLASH_BASE_ADDRESS, size);
}
