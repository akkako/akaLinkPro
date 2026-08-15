#include "api_param.h"
#include "usb_composite.h"
#include <string.h>

// #define FLASH_PAGE_SIZE (256)
// #define APP_PARAM_ADDR_OFFSET ((uint32_t)0x0801FF00)

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

api_param_t g_param;

void api_param_load(void)
{
    // memcpy(&g_param, (const void *)(APP_PARAM_ADDR_OFFSET), sizeof(app_param_t));
    if (g_param.magic_number != 0x0D000721)
    {
        // default value
        g_param.magic_number = 0x0D000721;
        g_param.output_mode = 0;
        g_param.swd_sim_mode = 0;
        g_param.usb5v_out_mode = 0;
        g_param.clock_accel_mode = 0;
        api_param_save();
    }
    else
    {
        if (g_param.usb5v_out_mode)
        {
            // drv_gpio_set_5ven();
        }
        else
        {
            // drv_gpio_reset_5ven();
        }
    }
}

void api_param_save(void)
{
    // uint32_t buf[FLASH_PAGE_SIZE / 4] = {0xFFFFFFFF};

    // memcpy(buf, &g_param, sizeof(app_param_t));

    // FLASH_Unlock_Fast();
    // FLASH_Access_Clock_Cfg(FLASH_Access_SYSTEM_HALF);

    // FLASH_ErasePage_Fast(APP_PARAM_ADDR_OFFSET);
    // FLASH_ProgramPage_Fast(APP_PARAM_ADDR_OFFSET, (uint32_t *)buf);

    // FLASH_Lock_Fast();
}

void api_param_proc_hid(uint8_t *req_hid, uint8_t *res_hid)
{
    uint8_t cmd = req_hid[2];
    switch (cmd)
    {
    case CMD_GET_CONFIG:
        res_hid[1] = 0x05;
        res_hid[2] = CMD_GET_CONFIG;
        res_hid[3] = g_param.output_mode;
        res_hid[4] = g_param.swd_sim_mode;
        res_hid[5] = g_param.usb5v_out_mode;
        res_hid[6] = g_param.clock_accel_mode;
        break;
    case CMD_SET_CONFIG:
        g_param.output_mode = req_hid[3] ? 0x01 : 0x00;
        g_param.swd_sim_mode = req_hid[4] ? 0x01 : 0x00;
        g_param.usb5v_out_mode = req_hid[5] ? 0x01 : 0x00;
        g_param.clock_accel_mode = req_hid[6] ? 0x01 : 0x00;
        res_hid[1] = 1;
        res_hid[2] = CMD_SET_CONFIG;
        break;
    case CMD_GET_VOLTAGE:
    {
        uint16_t vol = 3328;
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
        // app_param_save();
        res_hid[1] = 0x01;
        res_hid[2] = CMD_SAVE_CONFIG;
        break;
    case CMD_RESET_DEVICE:
        // NVIC_SystemReset();
        break;
    case CMD_ENTER_DFU:
        // drv_bkp_write_reg(0x0721);
        // NVIC_SystemReset();
        break;
    default:
        res_hid[1] = 0x01;
        res_hid[2] = CMD_NOT_SUPPORT;
        break;
    }
}