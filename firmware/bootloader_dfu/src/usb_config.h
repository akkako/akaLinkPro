/*
 * CherryUSB Configuration for akaLinkPro Bootloader
 * Matches CDC demo pattern + HS support
 */
#ifndef USB_CONFIG_H
#define USB_CONFIG_H

#include "board.h"

/* ================ USB common ================ */
#define CONFIG_USB_PRINTF(...) // printf(__VA_ARGS__)
#define CONFIG_USB_DBG_LEVEL USB_DBG_INFO
#define CONFIG_USB_ALIGN_SIZE 4
#define USB_NOCACHE_RAM_SECTION __attribute__((section(".noncacheable")))

/* ================ USB Device ================ */
#define CONFIG_USBDEV_MAX_BUS 1
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN 4096
#define CONFIG_USBDEV_ADVANCE_DESC

#define CONFIG_USB_DEVICE 1
#define CONFIG_USB_DEVICE_DFU 1

#define CONFIG_USBHOST_MAX_RHPORTS 1
#define CONFIG_USBHOST_MAX_EXTHUBS 1
#define CONFIG_USBHOST_MAX_EHPORTS 4
#define CONFIG_USBHOST_MAX_INTERFACES 4
#define CONFIG_USBHOST_MAX_INTF_ALTSETTINGS 1
#define CONFIG_USBHOST_MAX_ENDPOINTS 4
#define CONFIG_USBHOST_DEV_NAMELEN 16

#define CONFIG_HPM_USBD_BASE HPM_USB0_BASE
#define CONFIG_HPM_USBD_IRQn IRQn_USB0

#endif
