/*
 * Shared low-level flash access for the bootloader (ROM API wrapper).
 * Used by both the DFU port and the virtual-FAT MSC upgrade path.
 */
#ifndef DFU_FLASH_PORT_H
#define DFU_FLASH_PORT_H

#include <stdint.h>
#include "hpm_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Read `size` bytes from flash address `addr` (XIP, cache coherent). */
hpm_stat_t boot_flash_read(uint32_t addr, void *buf, uint32_t size);

/* Erase `size` bytes at `addr` (addr/size must be sector aligned). */
hpm_stat_t boot_flash_erase(uint32_t addr, uint32_t size);

/* Program `size` bytes to `addr` (area must be erased first). */
hpm_stat_t boot_flash_program(uint32_t addr, const void *buf, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /* DFU_FLASH_PORT_H */
