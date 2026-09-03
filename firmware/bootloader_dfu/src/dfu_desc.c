/*
 * DFU USB descriptors for HPM DFU Bootloader
 * DfuSe protocol path: uses SDK hpm_dfu_port.c for flash callbacks.
 */
#include "usbd_core.h"
#include "usbd_dfu.h"
#include "usb_dfu.h"
#include "board.h"
#include "usb_config.h"
#include <stdio.h>
#include "hpm_otp_drv.h"

#define USBD_VID 0x0D28
#define USBD_PID 0x0205
#define USBD_MAX_POWER 250

#if defined(CONFIG_USB_DEVICE_FS) || defined(CONFIG_USB_DEVICE_FORCE_FULL_SPEED)
#undef CONFIG_USB_HS
#else
#define CONFIG_USB_HS
#endif

/* DFU functional descriptor is 9 bytes */
#define DFU_DESC_TOTAL_LEN (9 + 9)
#define USB_CONFIG_SIZE (9 + DFU_DESC_TOTAL_LEN)

static char flash_internal_desc_str[128] = {0};
static char device_serial_number_str[33] = {0};

/* ========== Device Descriptor ========== */
static const uint8_t device_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0200, 0x01)};

/* ========== Config Descriptor (DFU-only, single alt setting) ========== */
static const uint8_t config_descriptor[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x01, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    /* DFU Interface 0, Alt 0, DFU Mode */
    0x09, 0x04, 0x00, 0x00, 0x00,
    0xFE, 0x01, 0x02, /* class=APP_SPECIFIC, subclass=DFU, protocol=DFU_MODE */
    0x04,             /* iInterface = 4 -> DfuSe memory layout string */
    /* DFU Functional Descriptor */
    0x09, 0x21, /* bLength, bDescriptorType = DFU_FUNCTIONAL */
    0x0B,       /* bmAttributes: CanDnload | CanUpload | WillDetach */
    0xFF, 0x00, /* wDetachTimeout = 255ms */
    0x00, 0x10, /* wTransferSize = 4096 */
    0x1A, 0x01, /* bcdDFU = 1.1a */
};

/* ========== String Descriptors ========== */
static const char *string_descriptors[] = {
    (const char[]){0x09, 0x04},
    "ARM",
    "akaLinkPro CMSIS-DAP DFU",
};
static const uint8_t *device_descriptor_cb(uint8_t speed)
{
    (void)speed;
    return device_descriptor;
}

static const uint8_t *config_descriptor_cb(uint8_t speed)
{
    (void)speed;
    return config_descriptor;
}

static const char *string_descriptor_cb(uint8_t speed, uint8_t index)
{
    (void)speed;
    if (index < (sizeof(string_descriptors) / sizeof(char *)))
    {
        return string_descriptors[index];
    }
    else if (index == 3)
    {
        return device_serial_number_str;
    }
    else if (index == 4)
    {
        return flash_internal_desc_str; /* filled by dfu_boot_init */
    }
    else
    {
        return NULL;
    }
}

/* ========== Device Qualifier (USB 2.0) ========== */
static const uint8_t device_quality_descriptor[] = {
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x01,
    0x00,
};

static const uint8_t *device_quality_descriptor_cb(uint8_t speed)
{
    (void)speed;
    return device_quality_descriptor;
}

/* ========== BOS Descriptor (USB 2.0 Extension) ========== */
static const uint8_t bos_descriptor_data[] = {
    /* BOS Header */
    0x05,
    USB_DESCRIPTOR_TYPE_BINARY_OBJECT_STORE,
    0x0C,
    0x00,
    0x01,
    /* USB 2.0 Extension Capability */
    0x07,
    0x10,
    0x02,
    0x02,
    0x00,
    0x00,
    0x00,
};

static const struct usb_bos_descriptor bos_descriptor = {
    .string = bos_descriptor_data,
    .string_len = sizeof(bos_descriptor_data),
};

/* ========== Descriptor Registration ========== */
const struct usb_descriptor dfu_descriptor = {
    .device_descriptor_callback = device_descriptor_cb,
    .config_descriptor_callback = config_descriptor_cb,
    .device_quality_descriptor_callback = device_quality_descriptor_cb,
    .other_speed_descriptor_callback = config_descriptor_cb,
    .string_descriptor_callback = string_descriptor_cb,
    .msosv2_descriptor = NULL,
    .bos_descriptor = &bos_descriptor,
};

/* ========== USB Init ========== */
static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    switch (event)
    {
    case USBD_EVENT_RESET:
        break;
    case USBD_EVENT_CONNECTED:
        break;
    case USBD_EVENT_CONFIGURED:
        break;
    default:
        break;
    }
}

static void get_device_serial_number(void)
{
#define OTP_UID_ADDR (88)
#define UID_WORD_COUNT (4)
#define UID_BYTE_COUNT (UID_WORD_COUNT * 4)

    uint32_t uid_words[UID_WORD_COUNT];
    uint8_t *uid_bytes = (uint8_t *)uid_words;

    // read UID data
    for (int i = 0; i < UID_WORD_COUNT; i++)
    {
        uid_words[i] = otp_read_from_ip(OTP_UID_ADDR + i);
    }

    // format to Hex string
    char *ptr = device_serial_number_str;
    for (int i = 0; i < UID_BYTE_COUNT; i++)
    {
        snprintf(ptr, 3, "%02X", uid_bytes[i]);
        ptr += 2;
    }
}

static struct usbd_interface intf0;

void dfu_boot_init(uint8_t busid, uintptr_t reg_base)
{
    /* Build DfuSe interface string at runtime (like SDK sample) */
    uint32_t dfu_app_size = BOARD_FLASH_SIZE - (USBD_DFU_APP_DEFAULT_ADD - BOARD_FLASH_BASE_ADDRESS);
    (void)snprintf(flash_internal_desc_str, sizeof(flash_internal_desc_str),
                   "@Internal Flash /0x%08lX/%lu*%luKg",
                   (unsigned long)USBD_DFU_APP_DEFAULT_ADD,
                   (unsigned long)(dfu_app_size / (16 * 1024)),
                   (unsigned long)(16));

    get_device_serial_number();

    usbd_desc_register(busid, &dfu_descriptor);
    usbd_add_interface(busid, usbd_dfu_init_intf(&intf0));
    usbd_initialize(busid, reg_base, usbd_event_handler);
}
