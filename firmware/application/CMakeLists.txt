# Copyright (c) 2021 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

set(APP_NAME akaLinkPro_App)

set(BOARD_SEARCH_PATH ${CMAKE_CURRENT_SOURCE_DIR}/boards)

# Prevent SDK from forcing -u _printf_float/-u _scanf_float (saves ~31KB)
set(HPM_SDK_LD_NO_NANO_SPECS 1)

# Enable CherryUSB device stack. CDC ACM + HID + MSC classes are needed for the
# CMSIS-DAP composite enumeration (CDC and custom-HID + MSC interfaces).
set(CONFIG_HPM_SPI 1)
set(CONFIG_DMA_MGR 1)
set(CONFIG_CHERRYRB 1)
set(CONFIG_CHERRYUSB 1)
set(CONFIG_CHERRYUSB_DEVICE 1)
set(CONFIG_USB_DEVICE 1)
set(CONFIG_USB_DEVICE_CDC_ACM 1)
set(CONFIG_USB_DEVICE_HID 1)
set(CONFIG_USB_DEVICE_MSC 1)
# NOTE: Do NOT set CONFIG_USB_DEVICE_DFU here.
# The APP only needs DFU runtime (usbd_core), not DFU mode (hpm_dfu_port.c).
# dap_main.c handles DFU DETACH/GETSTATUS/GETSTATE manually as a runtime iface.

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})
project(akaLinkPro_App)

# Use nano libc (saves ~31KB)
sdk_ld_options("--specs=nano.specs")

# CherryUSB config — pick up local usb_config.h
sdk_compile_definitions(-D__INCLUDE_CHERRYUSB_CONFIG_H__)

# Use SDK's flash_dfu.ld — reserve 128K for bootloader
sdk_linker_global_symbols("_dfu_bl_length=0x20000")

# sdk_compile_definitions(-DCONFIG_NDEBUG_CONSOLE=1)

sdk_inc(${CMAKE_CURRENT_SOURCE_DIR}/src/dfu)
sdk_inc(${CMAKE_CURRENT_SOURCE_DIR}/src/usb)
sdk_inc(${CMAKE_CURRENT_SOURCE_DIR}/src/led)
sdk_inc(${CMAKE_CURRENT_SOURCE_DIR}/src/dap)
sdk_inc(${CMAKE_CURRENT_SOURCE_DIR}/src/api)
sdk_inc(${CMAKE_CURRENT_SOURCE_DIR}/src/dap)

sdk_app_src(src/main.c)
sdk_app_src(src/led/ws2812.c)
sdk_app_src(src/dfu/hpm_dfu_trigger.c)
sdk_app_src(src/usb/usb_composite.c)
sdk_app_src(src/usb/hid_interface.c)
sdk_app_src(src/usb/cdc_interface.c)
sdk_app_src(src/usb/msc_interface.c)
sdk_app_src(src/api/api_param.c)
sdk_app_src(src/dap/DAP.c)
sdk_app_src(src/dap/SW_DP/SW_DP.c)
sdk_app_src(src/dap/JTAG_DP/JTAG_DP.c)

# generate_ide_projects()
