/*
 * Virtual FAT16 volume for the bootloader U-disk (see vfat.h).
 */
#include "vfat.h"

#include <string.h>
#include <stdio.h>
#include "board.h"
#include "hpm_otp_drv.h"
#include "hpm_ewdg_drv.h"
#include "hpm_dfu_trigger.h"
#include "dfu_flash_port.h"

#define SECTOR_SIZE           (512u)
#define TOTAL_SECTORS         (0x40000u)   /* 128 MB reported (actual flashable < 1 MB) */
#define SPC                   (64u)        /* sectors per cluster = 32 KB */
#define RESERVED_SECTORS      (1u)
#define NUM_FATS              (2u)
#define FAT_SIZE_SECTORS      (17u)
#define ROOT_ENTRIES          (256u)
#define ROOT_SECTORS          ((ROOT_ENTRIES * 32u) / SECTOR_SIZE)  /* 16 */
#define DATA_START_LBA        (RESERVED_SECTORS + NUM_FATS * FAT_SIZE_SECTORS + ROOT_SECTORS)
#define CLUSTER_SIZE          (SPC * SECTOR_SIZE)

#define CLUSTER_INFO          (2u)
#define CLUSTER_URL           (3u)
#define CLUSTER_HTM           (4u)
#define CLUSTER_UPGRADE       (5u)

#define APP_BASE              (0x80020000u)
#define APP_END               (0x800FE000u)  /* never touch the EasyFlash tail */
#define APP_MAX_SIZE          (APP_END - APP_BASE)

#define FLASH_SECTOR          (0x1000u)

static uint8_t s_boot[SECTOR_SIZE];
static uint8_t s_fat1[SECTOR_SIZE];
static uint8_t s_fat2[SECTOR_SIZE];
static uint8_t s_root[ROOT_SECTORS * SECTOR_SIZE];
static char    s_info[512];

static const char s_url[] =
    "[InternetShortcut]\r\n"
    "URL=https://akkako.github.io/akaLinkPro/\r\n";

static const char s_htm[] =
    "<html><head><meta http-equiv=\"refresh\" content=\"0;url=https://akkako.github.io/akaLinkPro/\">"
    "</head><body>Redirecting to akaLinkPro...</body></html>";

static uint32_t s_info_len;
static uint32_t s_url_len;
static uint32_t s_htm_len;

static uint32_t s_upgrade_size;
static uint32_t s_upgrade_written;
static uint32_t s_flash_erased;
static uint16_t s_upgrade_start;   /* start cluster of the dragged *.BIN (from root dir) */
static uint8_t  s_upgrade_error;
static volatile uint8_t s_finished;

static void put16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static void put32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static void read_flash_str(char *dst, uint32_t addr, int max)
{
    char tmp[64];
    int n = (max < 63) ? max : 63;

    boot_flash_read(addr, tmp, (uint32_t)n);
    int i = 0;
    for (; i < n && tmp[i] != '\0'; i++) {
        dst[i] = tmp[i];
    }
    dst[i] = '\0';
}

static void build_sn(char *out)
{
    static const char hex[] = "0123456789ABCDEF";
    uint32_t w[4];
    uint8_t *b = (uint8_t *)w;

    for (int i = 0; i < 4; i++) {
        w[i] = otp_read_from_ip(88 + i);
    }
    for (int i = 0; i < 16; i++) {
        out[i * 2] = hex[(b[i] >> 4) & 0xF];
        out[i * 2 + 1] = hex[b[i] & 0xF];
    }
    out[32] = '\0';
}

static void build_info(void)
{
    char sn[33];
    char fwver[9] = "Unknown";
    char desc[25] = "Unknown";
    char blver[9] = "Unknown";
    char hwver[9] = "Unknown";

    build_sn(sn);
    read_flash_str(fwver, APP_BASE + 0x10, 8);
    read_flash_str(desc, APP_BASE + 0x2C, 24);
    read_flash_str(blver, 0x8001F000u + 0x10, 8);
    read_flash_str(hwver, 0x8001F000u + 0x2C, 8);

    snprintf(s_info, sizeof(s_info),
             "SN:%s\r\n"
             "HWVER:%s\r\n"
             "BLVER:%s\r\n"
             "FWVER:%s\r\n"
             "DESC:%s\r\n"
             "CRC32 Check:%s\r\n"
             "Please drag a .bin firmware here to upgrade.\r\n",
             sn, hwver, blver, fwver, desc,
             hpm_dfu_app_valid() ? "pass" : "fail");
    s_info_len = (uint32_t)strlen(s_info);
}

static void dir_entry(uint8_t *e, const char name[11], uint8_t attr, uint32_t cluster, uint32_t size)
{
    uint16_t date = (uint16_t)(((2026 - 1980) << 9) | (1 << 5) | 1);

    memset(e, 0, 32);
    memcpy(e, name, 11);
    e[11] = attr;
    put16(&e[16], date);   /* creation date */
    put16(&e[18], date);   /* last access date */
    put16(&e[24], date);   /* write date */
    put16(&e[26], (uint16_t)cluster);
    put32(&e[28], size);
}

static void build_root(void)
{
    static const char vol[]  = {'A','K','A','L','I','N','K','P','R','O',' '};
    static const char info[] = {'I','N','F','O',' ',' ',' ',' ','T','X','T'};
    static const char url[]  = {'A','K','A','L','I','N','K',' ','U','R','L'};
    static const char htm[]  = {'A','K','A','L','I','N','K',' ','H','T','M'};

    memset(s_root, 0, sizeof(s_root));
    dir_entry(&s_root[0],  vol,  0x08, 0, 0);
    dir_entry(&s_root[32], info, 0x20, CLUSTER_INFO, s_info_len);
    dir_entry(&s_root[64], url,  0x20, CLUSTER_URL, s_url_len);
    dir_entry(&s_root[96], htm,  0x20, CLUSTER_HTM, s_htm_len);
}

static void build_fat(void)
{
    memset(s_fat1, 0, sizeof(s_fat1));
    put16(&s_fat1[0], 0xFFF8);
    put16(&s_fat1[2], 0xFFFF);
    put16(&s_fat1[CLUSTER_INFO * 2], 0xFFFF);
    put16(&s_fat1[CLUSTER_URL * 2], 0xFFFF);
    put16(&s_fat1[CLUSTER_HTM * 2], 0xFFFF);
    memcpy(s_fat2, s_fat1, sizeof(s_fat1));
}

static void build_boot(void)
{
    memset(s_boot, 0, sizeof(s_boot));
    s_boot[0] = 0xEB;
    s_boot[1] = 0x3C;
    s_boot[2] = 0x90;
    memcpy(&s_boot[3], "MSDOS5.0", 8);
    put16(&s_boot[11], SECTOR_SIZE);
    s_boot[13] = SPC;
    put16(&s_boot[14], RESERVED_SECTORS);
    s_boot[16] = NUM_FATS;
    put16(&s_boot[17], ROOT_ENTRIES);
    put16(&s_boot[19], 0);              /* total sectors 16 = 0 (use 32-bit field) */
    s_boot[21] = 0xF8;
    put16(&s_boot[22], FAT_SIZE_SECTORS);
    put16(&s_boot[24], 32);
    put16(&s_boot[26], 2);
    put32(&s_boot[28], 0);              /* hidden sectors */
    put32(&s_boot[32], TOTAL_SECTORS);
    s_boot[36] = 0x80;
    s_boot[38] = 0x29;
    put32(&s_boot[39], 0x12345678);
    memcpy(&s_boot[43], "AKALINKPRO ", 11);
    memcpy(&s_boot[54], "FAT16   ", 8);
    s_boot[510] = 0x55;
    s_boot[511] = 0xAA;
}

void vfat_init(void)
{
    s_upgrade_size = 0;
    s_upgrade_written = 0;
    s_flash_erased = 0;
    s_upgrade_start = 0;
    s_upgrade_error = 0;
    s_finished = 0;

    s_url_len = (uint32_t)sizeof(s_url) - 1;
    s_htm_len = (uint32_t)sizeof(s_htm) - 1;

    build_info();
    build_root();
    build_fat();
    build_boot();
}

static void copy_from(const uint8_t *src, uint32_t src_len, uint32_t off, uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        uint32_t p = off + i;
        buf[i] = (p < src_len) ? src[p] : 0x00;
    }
}

void vfat_read_sector(uint32_t lba, uint8_t *buf, uint32_t len)
{
    memset(buf, 0, len);
    if (lba >= TOTAL_SECTORS) {
        return;
    }
    if (lba == 0) {
        copy_from(s_boot, SECTOR_SIZE, 0, buf, len);
        return;
    }
    if (lba < RESERVED_SECTORS + FAT_SIZE_SECTORS) {
        if ((lba - RESERVED_SECTORS) == 0) {
            copy_from(s_fat1, SECTOR_SIZE, 0, buf, len);
        }
        return;
    }
    if (lba < RESERVED_SECTORS + NUM_FATS * FAT_SIZE_SECTORS) {
        if ((lba - RESERVED_SECTORS - FAT_SIZE_SECTORS) == 0) {
            copy_from(s_fat2, SECTOR_SIZE, 0, buf, len);
        }
        return;
    }
    if (lba < DATA_START_LBA) {
        uint32_t off = (lba - (RESERVED_SECTORS + NUM_FATS * FAT_SIZE_SECTORS)) * SECTOR_SIZE;
        copy_from(s_root, sizeof(s_root), off, buf, len);
        return;
    }

    {
        uint32_t rel = lba - DATA_START_LBA;
        uint32_t cluster = CLUSTER_INFO + rel / SPC;
        uint32_t off = (rel % SPC) * SECTOR_SIZE;

        if (cluster == CLUSTER_INFO) {
            copy_from((const uint8_t *)s_info, s_info_len, off, buf, len);
        } else if (cluster == CLUSTER_URL) {
            copy_from((const uint8_t *)s_url, s_url_len, off, buf, len);
        } else if (cluster == CLUSTER_HTM) {
            copy_from((const uint8_t *)s_htm, s_htm_len, off, buf, len);
        } else {
            uint32_t foff = (cluster - CLUSTER_UPGRADE) * CLUSTER_SIZE + off;
            if (foff < APP_MAX_SIZE) {
                boot_flash_read(APP_BASE + foff, buf, len);
            }
        }
    }
}

static void ensure_erased(uint32_t end_off)
{
    while (s_flash_erased < end_off && s_flash_erased < APP_MAX_SIZE) {
        ewdg_refresh(HPM_EWDG0);
        if (boot_flash_erase(APP_BASE + s_flash_erased, FLASH_SECTOR) != status_success) {
            s_upgrade_error = 1;
            return;
        }
        s_flash_erased += FLASH_SECTOR;
    }
}

static void scan_root(const uint8_t *dir, uint32_t bytes)
{
    for (uint32_t i = 0; i + 32 <= bytes; i += 32) {
        const uint8_t *e = &dir[i];
        if (e[0] == 0x00) {
            break;
        }
        if (e[0] == 0xE5 || e[11] == 0x0F || e[11] == 0x08) {
            continue;
        }
        if (e[11] != 0x20) {
            continue;
        }
        if ((e[8] | 0x20) == 'b' && (e[9] | 0x20) == 'i' && (e[10] | 0x20) == 'n') {
            uint16_t start = (uint16_t)(e[26] | ((uint16_t)e[27] << 8));
            uint32_t size = (uint32_t)e[28] | ((uint32_t)e[29] << 8)
                          | ((uint32_t)e[30] << 16) | ((uint32_t)e[31] << 24);

            if (start != s_upgrade_start) {
                /* A new (or re-created) upgrade file starts here. */
                s_upgrade_start = start;
                s_upgrade_written = 0;
                s_finished = 0;
                s_upgrade_error = 0;
            }
            if (size != 0) {
                if (size > APP_MAX_SIZE) {
                    s_upgrade_error = 1;
                }
                s_upgrade_size = size;
            }
            if ((s_upgrade_size != 0) && (s_upgrade_written >= s_upgrade_size) && (s_upgrade_error == 0)) {
                s_finished = 1;
            }
        }
    }
}

/* Next cluster in the FAT16 chain (only the first FAT sector is tracked). */
static uint16_t fat_next(uint16_t cluster)
{
    if (cluster < 2) {
        return 0xFFFF;
    }
    return (uint16_t)(s_fat1[cluster * 2] | ((uint16_t)s_fat1[cluster * 2 + 1] << 8));
}

void vfat_write_sector(uint32_t lba, const uint8_t *buf, uint32_t len)
{
    if (lba >= TOTAL_SECTORS) {
        return;
    }

    /* FAT (only the first sector holds our small cluster range). */
    if (lba >= RESERVED_SECTORS && lba < RESERVED_SECTORS + FAT_SIZE_SECTORS) {
        if ((lba - RESERVED_SECTORS) == 0) {
            memcpy(s_fat1, buf, len < SECTOR_SIZE ? len : SECTOR_SIZE);
        }
        return;
    }
    if (lba >= RESERVED_SECTORS + FAT_SIZE_SECTORS && lba < RESERVED_SECTORS + NUM_FATS * FAT_SIZE_SECTORS) {
        if ((lba - RESERVED_SECTORS - FAT_SIZE_SECTORS) == 0) {
            memcpy(s_fat2, buf, len < SECTOR_SIZE ? len : SECTOR_SIZE);
        }
        return;
    }

    /* Root directory: capture a newly created *.BIN. */
    if (lba >= RESERVED_SECTORS + NUM_FATS * FAT_SIZE_SECTORS && lba < DATA_START_LBA) {
        uint32_t off = (lba - (RESERVED_SECTORS + NUM_FATS * FAT_SIZE_SECTORS)) * SECTOR_SIZE;
        if (off + len <= sizeof(s_root)) {
            memcpy(&s_root[off], buf, len);
        }
        scan_root(s_root, sizeof(s_root));
        return;
    }

    /* Data area: only clusters in the dragged *.BIN chain go to APP flash. */
    if (lba >= DATA_START_LBA) {
        uint32_t rel = lba - DATA_START_LBA;
        uint16_t cluster = (uint16_t)(CLUSTER_INFO + rel / SPC);
        uint32_t off = (rel % SPC) * SECTOR_SIZE;
        uint32_t idx = 0;
        uint16_t cur;

        if (s_upgrade_start < 2) {
            return; /* no upgrade file created yet */
        }

        /* Find this cluster's position in the file chain (skips other files). */
        cur = s_upgrade_start;
        while ((cur != cluster) && (idx < 0xFFF0u)) {
            uint16_t nxt = fat_next(cur);
            if ((nxt < 2) || (nxt >= 0xFFF8)) {
                break;
            }
            cur = nxt;
            idx++;
        }
        if (cur != cluster) {
            return; /* not part of the firmware file (e.g. System Volume Information) */
        }

        uint32_t foff = idx * CLUSTER_SIZE + off;
        if (foff >= APP_MAX_SIZE) {
            s_upgrade_error = 1;
            return;
        }

        uint32_t n = len;
        if (foff + n > APP_MAX_SIZE) {
            n = APP_MAX_SIZE - foff;
        }
        ensure_erased(foff + n);
        if (s_upgrade_error) {
            return;
        }
        ewdg_refresh(HPM_EWDG0);
        if (boot_flash_program(APP_BASE + foff, buf, n) != status_success) {
            s_upgrade_error = 1;
            return;
        }
        if (foff + n > s_upgrade_written) {
            s_upgrade_written = foff + n;
        }
        if (s_upgrade_size != 0 && s_upgrade_written >= s_upgrade_size) {
            s_finished = 1;
        }
    }
}

bool vfat_upgrade_finished(void)
{
    return (s_finished != 0) && (s_upgrade_error == 0);
}
