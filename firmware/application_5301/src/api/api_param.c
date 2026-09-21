#include "api_param.h"
#include "usb_composite.h"
#include "led_state.h"
#include "hpm_dfu_trigger.h"
#include "board.h"

#include <string.h>
#include <stddef.h>
#include "hpm_common.h"
#include "hpm_romapi.h"
#include "hpm_l1c_drv.h"
#include "hpm_ppor_drv.h"
#include "hpm_crc32.h"

#define BOOTLOADER_START_ADDR (0x00000000)
#define BOOTLOADER_VER_STR_ADDR "1.0"
#define BOOTLOADER_TS_STR_ADDR "2026-08-06 08:22:34"

#define HARDWARE_VER_STR_ADDR "A.0"
#define HARDWARE_PROD_TS_STR_ADDR "2026-08-06"

#define APPLICATION_CODE_LENGTH_ADDR (0x00004000)
#define APPLICATION_CODE_CRC32_ADDR (0x00004004)
#define APPLICATION_VER_STR_ADDR "0.1"
#define APPLICATION_TS_STR_ADDR "2026-08-06 11:32:45"
#define APPLICATION_DESC_STR_ADDR "akaLinkPro CMSIS-DAP"

#define CMD_NOT_SUPPORT (0x00)
#define CMD_GET_CONFIG (0x01)
#define CMD_SET_CONFIG (0x02)
#define CMD_GET_VOLTAGE (0x03)
#define CMD_SAVE_CONFIG (0x04)
#define CMD_GET_MODEL (0x10)
#define CMD_GET_SERIAL (0x11)
#define CMD_GET_HWVER (0x12)
#define CMD_GET_FWVER (0x13)
#define CMD_GET_BLVER (0x14)
#define CMD_GET_HW_PROD_DATE (0x15)
#define CMD_GET_FW_COMPILE_DATE (0x16)
#define CMD_GET_BL_COMPILE_DATE (0x17)
#define CMD_RESET_DEVICE (0xFE)
#define CMD_ENTER_DFU (0xFF)

#define PARAM_MAGIC_NUMBER (0x0D000721UL)
#define PARAM_STORE_MAGIC (0x30444150UL) /* "PAD0" */
#define PARAM_STORE_VERSION (1UL)

/* Two 4K sectors reserved at the tail of the APP flash region (never in the
 * bootloader region). The config is written ping-pong between them with an
 * increasing sequence number: a power loss during a save always leaves the
 * previous copy intact, so at least one slot is valid. */
#define PARAM_SECTOR_SIZE (0x1000UL)
#define PARAM_SLOT0_ADDR (BOARD_FLASH_BASE_ADDRESS + BOARD_FLASH_SIZE - (2UL * PARAM_SECTOR_SIZE))
#define PARAM_SLOT1_ADDR (BOARD_FLASH_BASE_ADDRESS + BOARD_FLASH_SIZE - PARAM_SECTOR_SIZE)

#define VREF_MV_MIN (1800U)
#define VREF_MV_MAX (5000U)

typedef struct {
    uint32_t magic;
    uint32_t crc; /* CRC32 over version .. end of struct */
    uint32_t version;
    uint32_t seq;
    api_param_t param;
} param_store_t;

api_param_t g_param;

static volatile uint8_t s_save_pending;
static xpi_nor_config_t s_nor_config;
static uint8_t s_nor_ready;
static uint8_t s_active_slot; /* 0/1: slot that currently holds the newest copy */
static uint32_t s_seq;        /* sequence number of the active slot */

static uint16_t clamp_vref(uint16_t v)
{
    if (v < VREF_MV_MIN)
    {
        return VREF_MV_MIN;
    }
    if (v > VREF_MV_MAX)
    {
        return VREF_MV_MAX;
    }
    return v;
}

static uint8_t clamp_led_mode(uint8_t m)
{
    if ((m < LED_MODE_DAP_RUNNING) || (m > LED_MODE_OFF))
    {
        return LED_MODE_OFF;
    }
    return m;
}

static hpm_stat_t param_flash_init(void)
{
    if (s_nor_ready)
    {
        return status_success;
    }

    xpi_nor_config_option_t option;
    option.header.U = BOARD_APP_XPI_NOR_CFG_OPT_HDR;
    option.option0.U = BOARD_APP_XPI_NOR_CFG_OPT_OPT0;
    option.option1.U = BOARD_APP_XPI_NOR_CFG_OPT_OPT1;

    hpm_stat_t status = rom_xpi_nor_auto_config((XPI_Type *)BOARD_APP_XPI_NOR_XPI_BASE,
                                                &s_nor_config, &option);
    if (status == status_success)
    {
        s_nor_ready = 1;
    }
    return status;
}

static uint8_t param_store_valid(const param_store_t *store)
{
    uint32_t crc;

    if ((store->magic != PARAM_STORE_MAGIC) || (store->version != PARAM_STORE_VERSION))
    {
        return 0;
    }
    crc = crc32((const uint8_t *)&store->version,
                sizeof(*store) - offsetof(param_store_t, version));
    return (crc == store->crc) ? 1U : 0U;
}

static void param_flash_read(uint32_t addr, void *buf, uint32_t size)
{
    uint32_t aligned_start = HPM_L1C_CACHELINE_ALIGN_DOWN(addr);
    uint32_t aligned_end = HPM_L1C_CACHELINE_ALIGN_UP(addr + size);

    l1c_dc_invalidate(aligned_start, aligned_end - aligned_start);
    memcpy(buf, (const void *)addr, size);
}

static hpm_stat_t param_flash_write(uint32_t addr, const void *buf, uint32_t size)
{
    uint32_t offset = addr - BOARD_FLASH_BASE_ADDRESS;
    XPI_Type *base = (XPI_Type *)BOARD_APP_XPI_NOR_XPI_BASE;

    hpm_stat_t status = rom_xpi_nor_erase(base, xpi_xfer_channel_auto,
                                          &s_nor_config, offset, PARAM_SECTOR_SIZE);
    if (status != status_success)
    {
        return status;
    }

    status = rom_xpi_nor_program(base, xpi_xfer_channel_auto, &s_nor_config,
                                 (const uint32_t *)buf, offset, size);
    fencei();
    return status;
}

void api_param_apply(void)
{
    board_set_5v_output(g_param.usb5v_out_mode ? 1U : 0U);
}

void api_param_load(void)
{
    static param_store_t s0;
    static param_store_t s1;
    uint8_t v0;
    uint8_t v1;

    param_flash_read(PARAM_SLOT0_ADDR, &s0, sizeof(s0));
    param_flash_read(PARAM_SLOT1_ADDR, &s1, sizeof(s1));
    v0 = param_store_valid(&s0);
    v1 = param_store_valid(&s1);

    if (v0 && (!v1 || (s0.seq >= s1.seq)))
    {
        g_param = s0.param;
        s_active_slot = 0U;
        s_seq = s0.seq;
    }
    else if (v1)
    {
        g_param = s1.param;
        s_active_slot = 1U;
        s_seq = s1.seq;
    }
    else
    {
        g_param.magic_number = PARAM_MAGIC_NUMBER;
        g_param.output_mode = 0;
        g_param.usb5v_out_mode = 1; /* level-shifter supply, on by default */
        g_param.clock_accel_mode = 0;
        g_param.led1_mode = LED_MODE_POWER; /* LED1: debugger power, always on */
        g_param.led2_mode = LED_MODE_VREF;  /* LED2: external reference detection */
        g_param.vref_mv = 3300;
        s_active_slot = 0U;
        s_seq = 0U;
        api_param_save(); /* stores into slot 1 */
    }

    api_param_apply();
}

void api_param_save(void)
{
    static param_store_t store;
    uint8_t slot;
    uint32_t addr;
    hpm_stat_t status;

    if (param_flash_init() != status_success)
    {
        return;
    }

    /* Ping-pong: always write the slot other than the active one so the current
     * copy survives an interrupted save. */
    slot = s_active_slot ^ 1U;
    addr = (slot == 0U) ? PARAM_SLOT0_ADDR : PARAM_SLOT1_ADDR;

    store.magic = PARAM_STORE_MAGIC;
    store.version = PARAM_STORE_VERSION;
    store.seq = s_seq + 1U;
    store.param = g_param;
    store.crc = crc32((const uint8_t *)&store.version,
                      sizeof(store) - offsetof(param_store_t, version));

    uint32_t level = disable_global_irq(CSR_MSTATUS_MIE_MASK);
    status = param_flash_write(addr, &store, sizeof(store));
    restore_global_irq(level);

    if (status == status_success)
    {
        s_active_slot = slot;
        s_seq = store.seq;
    }
}

void api_param_request_save(void)
{
    s_save_pending = 1;
}

void api_param_poll(void)
{
    if (s_save_pending)
    {
        s_save_pending = 0;
        api_param_save();
    }
}

void api_param_proc_hid(uint8_t *req_hid, uint8_t *res_hid)
{
    uint8_t cmd = req_hid[2];
    switch (cmd)
    {
    case CMD_GET_CONFIG:
        res_hid[1] = 0x07;
        res_hid[2] = CMD_GET_CONFIG;
        res_hid[3] = g_param.output_mode;
        res_hid[4] = g_param.usb5v_out_mode;
        res_hid[5] = g_param.clock_accel_mode;
        res_hid[6] = g_param.led1_mode;
        res_hid[7] = g_param.led2_mode;
        res_hid[8] = (uint8_t)(g_param.vref_mv & 0xFF);
        res_hid[9] = (uint8_t)((g_param.vref_mv >> 8) & 0xFF);
        break;
    case CMD_SET_CONFIG:
        g_param.output_mode = req_hid[3] ? 0x01 : 0x00;
        g_param.usb5v_out_mode = req_hid[4] ? 0x01 : 0x00;
        g_param.clock_accel_mode = req_hid[5] ? 0x01 : 0x00;
        g_param.led1_mode = clamp_led_mode(req_hid[6]);
        g_param.led2_mode = clamp_led_mode(req_hid[7]);
        g_param.vref_mv = clamp_vref((uint16_t)(req_hid[8] | ((uint16_t)req_hid[9] << 8)));
        api_param_apply();
        res_hid[1] = 1;
        res_hid[2] = CMD_SET_CONFIG;
        break;
    case CMD_GET_VOLTAGE:
    {
        uint16_t vol = led_state_get_external_mv(); /* measured target reference, mV */
        res_hid[1] = 3;
        res_hid[2] = CMD_GET_VOLTAGE;
        res_hid[3] = (vol >> 0) & 0xFF;
        res_hid[4] = (vol >> 8) & 0xFF;
    }
    break;
    case CMD_GET_MODEL:
        res_hid[0x01] = 1 + sizeof("akaLinkPro");
        res_hid[0x02] = CMD_GET_MODEL;
        strcpy((char*)&res_hid[0x03], "akaLinkPro");
        break;
    case CMD_GET_SERIAL:
        res_hid[0x01] = 0x0E;
        res_hid[0x02] = CMD_GET_SERIAL;
        strcpy((char*)&res_hid[0x03], serial_number_dynamic);
        break;
    case CMD_GET_HWVER:
        res_hid[0x01] = 0x06;
        res_hid[0x02] = CMD_GET_HWVER;
        strncpy((char*)&res_hid[0x03], (const char*)HARDWARE_VER_STR_ADDR, 4);
        res_hid[0x07] = 0x00;
        break;
    case CMD_GET_FWVER:
        res_hid[0x01] = 0x06;
        res_hid[0x02] = CMD_GET_FWVER;
        strncpy((char*)&res_hid[0x03], (const char*)APPLICATION_VER_STR_ADDR, 4);
        res_hid[0x07] = 0x00;
        break;
    case CMD_GET_BLVER:
        res_hid[0x01] = 0x06;
        res_hid[0x02] = CMD_GET_BLVER;
        strncpy((char*)&res_hid[0x03], (const char*)BOOTLOADER_VER_STR_ADDR, 4);
        res_hid[0x07] = 0x00;
        break;
    case CMD_GET_HW_PROD_DATE:
        res_hid[0x01] = 0x15;
        res_hid[0x02] = CMD_GET_HW_PROD_DATE;
        strncpy((char*)&res_hid[0x03], (const char*)HARDWARE_PROD_TS_STR_ADDR, 19);
        res_hid[0x16] = 0x00;
        break;
    case CMD_GET_FW_COMPILE_DATE:
        res_hid[0x01] = 0x15;
        res_hid[0x02] = CMD_GET_FW_COMPILE_DATE;
        strncpy((char*)&res_hid[0x03], (const char*)APPLICATION_TS_STR_ADDR, 19);
        res_hid[0x16] = 0x00;
        break;
    case CMD_GET_BL_COMPILE_DATE:
        res_hid[0x01] = 0x15;
        res_hid[0x02] = CMD_GET_BL_COMPILE_DATE;
        strncpy((char*)&res_hid[0x03], (const char*)BOOTLOADER_TS_STR_ADDR, 19);
        res_hid[0x16] = 0x00;
        break;
    case CMD_SAVE_CONFIG:
        api_param_request_save();
        res_hid[1] = 0x01;
        res_hid[2] = CMD_SAVE_CONFIG;
        break;
    case CMD_RESET_DEVICE:
        ppor_reset_mask_set_source_enable(HPM_PPOR, ppor_reset_software);
        ppor_sw_reset(HPM_PPOR, 24);
        break;
    case CMD_ENTER_DFU:
        hpm_dfu_reboot_to_dfu();
        break;
    default:
        res_hid[1] = 0x01;
        res_hid[2] = CMD_NOT_SUPPORT;
        break;
    }
}
