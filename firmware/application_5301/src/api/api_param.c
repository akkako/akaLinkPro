#include "api_param.h"
#include "usb_composite.h"
#include "led_state.h"
#include "hpm_dfu_trigger.h"
#include "board.h"

#include <string.h>
#include "hpm_common.h"
#include "hpm_ppor_drv.h"
#include "easyflash.h"

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
/* EasyFlash ENV key that stores the whole api_param_t blob. */
#define API_PARAM_ENV_KEY "cfg"

#define VREF_MV_MIN (1800U)
#define VREF_MV_MAX (5000U)

const api_param_t g_param_default = {
    .magic_number = PARAM_MAGIC_NUMBER,
    .output_mode = 0,
    .usb5v_out_mode = 1, /* level-shifter supply, on by default */
    .clock_accel_mode = 0,
    .led1_mode = LED_MODE_POWER, /* LED1: debugger power, always on */
    .led2_mode = LED_MODE_VREF,  /* LED2: external reference detection */
    .vref_mv = 3300,
};

api_param_t g_param;

static volatile uint8_t s_save_pending;

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

void api_param_apply(void)
{
    board_set_5v_output(g_param.usb5v_out_mode ? 1U : 0U);
}

void api_param_load(void)
{
    uint8_t loaded = 0U;

    if (easyflash_init() == EF_NO_ERR)
    {
        size_t saved_len = 0U;
        size_t read_len = ef_get_env_blob(API_PARAM_ENV_KEY, &g_param, sizeof(g_param), &saved_len);
        if ((read_len == sizeof(g_param)) && (g_param.magic_number == PARAM_MAGIC_NUMBER))
        {
            loaded = 1U;
        }
    }

    if (!loaded)
    {
        g_param = g_param_default;
        api_param_save();
    }

    /* Sanitize values coming from flash. */
    g_param.led1_mode = clamp_led_mode(g_param.led1_mode);
    g_param.led2_mode = clamp_led_mode(g_param.led2_mode);
    g_param.vref_mv = clamp_vref(g_param.vref_mv);

    api_param_apply();
}

void api_param_save(void)
{
    if (easyflash_init() != EF_NO_ERR)
    {
        return;
    }
    (void)ef_set_env_blob(API_PARAM_ENV_KEY, &g_param, sizeof(g_param));
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
