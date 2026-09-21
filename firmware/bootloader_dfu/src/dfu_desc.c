/*
 * DFU USB descriptors for HPM DFU Bootloader
 * DfuSe protocol path: uses SDK hpm_dfu_port.c for flash callbacks.
 */
#include "usbd_core.h"
#include "usbd_dfu.h"
#include "usbd_msc.h"
#include "usb_dfu.h"
#include "board.h"
#include "usb_config.h"
#include <stdio.h>
#include "hpm_otp_drv.h"
#include "vfat.h"

#define USBD_VID 0x0D28
/* Composite DFU+MSC uses a dedicated PID (0x0207) so Windows does not reuse a
 * stale device-level WinUSB binding (0x0205 was DFU-only). WinUSB for the DFU
 * interface is auto-installed via the MS OS 2.0 descriptors below. */
#define USBD_PID 0x0207
#define USBD_MAX_POWER 250

/* MSC interface endpoints. */
#define MSC_IN_EP  0x81
#define MSC_OUT_EP 0x02

#if defined(CONFIG_USB_DEVICE_FS) || defined(CONFIG_USB_DEVICE_FORCE_FULL_SPEED)
#undef CONFIG_USB_HS
#else
#define CONFIG_USB_HS
#endif

#ifdef CONFIG_USB_HS
#define MSC_MAX_MPS 512
#else
#define MSC_MAX_MPS 64
#endif

/* Microsoft OS 2.0 / WinUSB: let Windows auto-bind WinUSB to the DFU interface
 * (interface 0) so libusb / dfu-util can claim it, while MSC stays on usbstor. */
#define WINUSB_VENDOR_CODE 0x20

const uint8_t WINUSB_WCIDDescriptor[] = {
    USB_MSOSV2_COMP_ID_SET_HEADER_DESCRIPTOR_INIT(10 + USB_MSOSV2_COMP_ID_FUNCTION_WINUSB_MULTI_DESCRIPTOR_LEN),
    USB_MSOSV2_COMP_ID_FUNCTION_WINUSB_MULTI_DESCRIPTOR_INIT(0x00),
};

const struct usb_msosv2_descriptor msosv2_desc = {
    .vendor_code = WINUSB_VENDOR_CODE,
    .compat_id = WINUSB_WCIDDescriptor,
    .compat_id_len = sizeof(WINUSB_WCIDDescriptor),
};

/* DFU functional descriptor is 9 bytes */
#define DFU_DESC_TOTAL_LEN (9 + 9)
#define USB_CONFIG_SIZE (9 + DFU_DESC_TOTAL_LEN + MSC_DESCRIPTOR_LEN)

static char flash_internal_desc_str[128] = {0};
static char device_serial_number_str[33] = {0};

/* ========== Device Descriptor ========== */
static const uint8_t device_descriptor[] = {
    /* bcdUSB must be 0x0210 for Microsoft OS 2.0 descriptors. */
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_1, 0x00, 0x00, 0x00, USBD_VID, USBD_PID, 0x0200, 0x01)};

/* ========== Config Descriptor (DFU-only, single alt setting) ========== */
static const uint8_t config_descriptor[] = {
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    /* Interface 0: DFU, Alt 0, DFU Mode */
    0x09, 0x04, 0x00, 0x00, 0x00,
    0xFE, 0x01, 0x02, /* class=APP_SPECIFIC, subclass=DFU, protocol=DFU_MODE */
    0x04,             /* iInterface = 4 -> DfuSe memory layout string */
    /* DFU Functional Descriptor */
    0x09, 0x21, /* bLength, bDescriptorType = DFU_FUNCTIONAL */
    0x0B,       /* bmAttributes: CanDnload | CanUpload | WillDetach */
    0xFF, 0x00, /* wDetachTimeout = 255ms */
    0x00, 0x10, /* wTransferSize = 4096 */
    0x1A, 0x01, /* bcdDFU = 1.1a */
    /* Interface 1: MSC (virtual U-disk) */
    MSC_DESCRIPTOR_INIT(0x01, MSC_OUT_EP, MSC_IN_EP, MSC_MAX_MPS, 0x00),
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
    /* BOS Header + USB 2.0 Extension + WinUSB platform capability */
    USB_BOS_HEADER_DESCRIPTOR_INIT(5 + 7 + USB_BOS_CAP_PLATFORM_WINUSB_DESCRIPTOR_LEN, 2),
    /* USB 2.0 Extension Capability */
    0x07,
    0x10,
    0x02,
    0x02,
    0x00,
    0x00,
    0x00,
    /* WinUSB platform capability (points to the MSOSV2 descriptor set) */
    USB_BOS_CAP_PLATFORM_WINUSB_DESCRIPTOR_INIT(WINUSB_VENDOR_CODE, sizeof(WINUSB_WCIDDescriptor)),
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
    .msosv2_descriptor = &msosv2_desc,
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
static struct usbd_interface intf1;

void dfu_boot_init(uint8_t busid, uintptr_t reg_base)
{
    /* Build DfuSe interface string at runtime (like SDK sample).
     * Exclude the reserved parameter sectors at the tail of flash so dfu-util
     * never targets them. */
    uint32_t dfu_app_size = BOARD_FLASH_SIZE - (USBD_DFU_APP_DEFAULT_ADD - BOARD_FLASH_BASE_ADDRESS)
                          - BOARD_PARAM_RESERVED_SIZE;
    (void)snprintf(flash_internal_desc_str, sizeof(flash_internal_desc_str),
                   "@Internal Flash /0x%08lX/%lu*%luKg",
                   (unsigned long)USBD_DFU_APP_DEFAULT_ADD,
                   (unsigned long)(dfu_app_size / (16 * 1024)),
                   (unsigned long)(16));

    get_device_serial_number();

    /* Build the virtual U-disk contents before USB comes up. */
    vfat_init();

    usbd_desc_register(busid, &dfu_descriptor);
    usbd_add_interface(busid, usbd_dfu_init_intf(&intf0));
    usbd_add_interface(busid, usbd_msc_init_intf(busid, &intf1, MSC_OUT_EP, MSC_IN_EP));
    usbd_initialize(busid, reg_base, usbd_event_handler);
}
