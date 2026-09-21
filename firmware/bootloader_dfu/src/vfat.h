/*
 * Virtual FAT16 volume exposed as a USB MSC U-disk by the bootloader.
 *
 * Contains two read-only files (INFO.TXT, AKALINK.URL / AKALINK.HTM) and
 * accepts a dragged-in *.BIN firmware which is programmed directly into the
 * APP flash region (0x80020000..). The reported volume size is intentionally
 * larger than the real flashable space; writes beyond the APP region are
 * dropped so the reserved tail (EasyFlash @0x800FE000) is never touched.
 */
#ifndef __VFAT_H__
#define __VFAT_H__

#include <stdint.h>
#include <stdbool.h>

/* Logical block geometry reported to the MSC host. */
#define VFAT_SECTOR_SIZE  (512u)
#define VFAT_BLOCK_COUNT  (0x40000u)  /* 128 MB reported */

#ifdef __cplusplus
extern "C" {
#endif

/* Build the virtual volume (boot sector / FAT / root dir / file contents). */
void vfat_init(void);

/* MSC sector access (512-byte logical blocks). */
void vfat_read_sector(uint32_t lba, uint8_t *buf, uint32_t len);
void vfat_write_sector(uint32_t lba, const uint8_t *buf, uint32_t len);

/* True once a dragged *.BIN has been fully written to flash. */
bool vfat_upgrade_finished(void);

#ifdef __cplusplus
}
#endif

#endif /* __VFAT_H__ */
