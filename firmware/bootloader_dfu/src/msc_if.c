/*
 * USB MSC class callbacks backed by the virtual FAT volume (vfat.c).
 */
#include <stdint.h>
#include <stdbool.h>
#include "usbd_core.h"
#include "usbd_msc.h"
#include "vfat.h"

void usbd_msc_get_cap(uint8_t busid, uint8_t lun, uint32_t *block_num, uint32_t *block_size)
{
    (void)busid;
    (void)lun;
    *block_num = VFAT_BLOCK_COUNT;
    *block_size = VFAT_SECTOR_SIZE;
}

int usbd_msc_sector_read(uint8_t busid, uint8_t lun, uint32_t sector, uint8_t *buffer, uint32_t length)
{
    (void)busid;
    (void)lun;
    vfat_read_sector(sector, buffer, length);
    return 0;
}

int usbd_msc_sector_write(uint8_t busid, uint8_t lun, uint32_t sector, uint8_t *buffer, uint32_t length)
{
    (void)busid;
    (void)lun;
    vfat_write_sector(sector, buffer, length);
    return 0;
}
