/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) 2026 akaInstruments */

#include "drv_flash.h"
#include "board.h"

#include <string.h>
#include "hpm_romapi.h"
#include "hpm_l1c_drv.h"

static xpi_nor_config_t s_nor_config;
static uint8_t s_nor_ready;
static uint32_t s_sector_size;

hpm_stat_t drv_flash_init(void)
{
    xpi_nor_config_option_t option;

    if (s_nor_ready)
    {
        return status_success;
    }

    option.header.U = BOARD_APP_XPI_NOR_CFG_OPT_HDR;
    option.option0.U = BOARD_APP_XPI_NOR_CFG_OPT_OPT0;
    option.option1.U = BOARD_APP_XPI_NOR_CFG_OPT_OPT1;

    XPI_Type *base = (XPI_Type *)BOARD_APP_XPI_NOR_XPI_BASE;
    hpm_stat_t status = rom_xpi_nor_auto_config(base, &s_nor_config, &option);
    if (status == status_success)
    {
        (void)rom_xpi_nor_get_property(base, &s_nor_config,
                                       xpi_nor_property_sector_size, &s_sector_size);
        s_nor_ready = 1;
    }
    return status;
}

uint32_t drv_flash_sector_size(void)
{
    return s_sector_size;
}

hpm_stat_t drv_flash_read(uint32_t addr, void *buf, uint32_t size)
{
    uint32_t aligned_start = HPM_L1C_CACHELINE_ALIGN_DOWN(addr);
    uint32_t aligned_end = HPM_L1C_CACHELINE_ALIGN_UP(addr + size);

    /* Flash is memory-mapped; invalidate the D-cache so the read is coherent
     * after a previous program/erase. */
    l1c_dc_invalidate(aligned_start, aligned_end - aligned_start);
    memcpy(buf, (const void *)addr, size);
    return status_success;
}

hpm_stat_t drv_flash_erase(uint32_t addr, uint32_t size)
{
    hpm_stat_t status;

    if (drv_flash_init() != status_success)
    {
        return status_fail;
    }

    uint32_t offset = addr - BOARD_FLASH_BASE_ADDRESS;
    XPI_Type *base = (XPI_Type *)BOARD_APP_XPI_NOR_XPI_BASE;

    status = rom_xpi_nor_erase(base, xpi_xfer_channel_auto, &s_nor_config, offset, size);
    fencei();
    return status;
}

hpm_stat_t drv_flash_write(uint32_t addr, const void *buf, uint32_t size)
{
    const uint8_t *src = (const uint8_t *)buf;
    XPI_Type *base;

    if (drv_flash_init() != status_success)
    {
        return status_fail;
    }
    base = (XPI_Type *)BOARD_APP_XPI_NOR_XPI_BASE;

    /* Program word by word. Bytes outside the requested range are written as
     * 0xFF, which leaves already-erased/programmed bits untouched (NOR only
     * clears bits), so this is safe for arbitrary addr/size alignment. */
    while (size > 0U)
    {
        uint32_t word_addr = addr & ~3U;
        uint32_t byte_off = addr & 3U;
        uint32_t n = 4U - byte_off;
        uint32_t word = 0xFFFFFFFFU;
        uint32_t i;

        if (n > size)
        {
            n = size;
        }
        for (i = 0; i < n; i++)
        {
            uint32_t shift = 8U * (byte_off + i);
            word = (word & ~(0xFFUL << shift)) | ((uint32_t)src[i] << shift);
        }

        hpm_stat_t status = rom_xpi_nor_program(base, xpi_xfer_channel_auto, &s_nor_config,
                                                &word, word_addr - BOARD_FLASH_BASE_ADDRESS, 4U);
        if (status != status_success)
        {
            return status;
        }

        addr += n;
        src += n;
        size -= n;
    }

    fencei();
    return status_success;
}
