/*
 * Low-level QSPI NOR flash driver (ROM API wrapper).
 *
 * This is the hardware-facing layer ("drv"). It only knows about absolute
 * XIP addresses and byte buffers; nothing about any storage format.
 * Layering:  drv_flash  <-  EasyFlash port (ef_port.c)  <-  application (api_param).
 */
#ifndef __DRV_FLASH_H__
#define __DRV_FLASH_H__

#include <stdint.h>
#include "hpm_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the ROM API flash config. Idempotent. */
hpm_stat_t drv_flash_init(void);

/* Read `size` bytes from flash address `addr` into `buf` (cache-coherent). */
hpm_stat_t drv_flash_read(uint32_t addr, void *buf, uint32_t size);

/* Erase `size` bytes starting at `addr`. addr must be sector aligned. */
hpm_stat_t drv_flash_erase(uint32_t addr, uint32_t size);

/* Program `size` bytes from `buf` to `addr`. The target area must be erased.
 * Handles arbitrary address and size alignment (word based). */
hpm_stat_t drv_flash_write(uint32_t addr, const void *buf, uint32_t size);

/* NOR sector size in bytes (queried from ROM at init). */
uint32_t drv_flash_sector_size(void);

#ifdef __cplusplus
}
#endif

#endif /* __DRV_FLASH_H__ */
