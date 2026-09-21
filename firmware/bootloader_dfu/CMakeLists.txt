cmake_minimum_required(VERSION 3.13)

set(APP_NAME akaLinkPro_Boot)

# RISC-V architecture
set(RV_ARCH "rv32imac_zicsr_zifencei")
set(RV_ABI "ilp32")

# Prevent SDK from forcing -u _printf_float/-u _scanf_float (saves ~31KB)
set(HPM_SDK_LD_NO_NANO_SPECS 1)

# Enable CherryUSB with DFU (SDK DfuSe path: hpm_dfu_port.c handles flash, we provide descriptors)
set(CONFIG_HPM_SPI 1)
set(CONFIG_CHERRYUSB 1)
set(CONFIG_CHERRYUSB_DEVICE 1)
set(CONFIG_USB_DEVICE 1)
set(CONFIG_USB_DEVICE_DFU 1)
set(CONFIG_USB_DEVICE_MSC 1)
set(USBD_DFU_XFER_SIZE 4096)

# --- Board detection ---
if(NOT DEFINED BOARD)
    message(FATAL_ERROR
        "BOARD is not set. Use:\n"
        "  cmake --preset <preset>   (e.g. hpm5301evklite-boot)\n"
        "or\n"
        "  cmake -DBOARD=<board> ...\n"
    )
endif()

# --- Board search paths ---
if(NOT DEFINED BOARD_SEARCH_PATH AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/boards)
    set(BOARD_SEARCH_PATH ${CMAKE_CURRENT_SOURCE_DIR}/boards)
endif()

# Find HPM SDK
find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})
project(akaLinkPro_Boot)

# Replace the SDK's DFU flash port (hpm_dfu_port.c, pulled into the SDK library)
# with our own src/dfu_flash_port.c, which bounds every DFU erase/program to
# [APP_DEFAULT_ADD, BOARD_DFU_WRITABLE_END) and therefore can never touch the
# reserved parameter sectors at the tail of the flash.
get_target_property(_boot_sdk_srcs ${HPM_SDK_LIB} SOURCES)
if(_boot_sdk_srcs)
    list(FILTER _boot_sdk_srcs EXCLUDE REGEX "hpm_dfu_port")
    set_target_properties(${HPM_SDK_LIB} PROPERTIES SOURCES "${_boot_sdk_srcs}")
endif()

# Use nano libc (saves ~31KB)
sdk_ld_options("--specs=nano.specs")

# Bootloader: 128K flash using SDK's flash_xip.ld
sdk_linker_global_symbols("_flash_size=128K" "_noncacheable_size=256K")

# Include CherryUSB DFU class
sdk_inc($ENV{HPM_SDK_BASE}/middleware/cherryusb/class/dfu)

# Include paths
sdk_inc(src)

# CherryUSB config
sdk_compile_definitions(-D__INCLUDE_CHERRYUSB_CONFIG_H__)

# Source files — DFU-only bootloader
sdk_app_src(src/main.c)
sdk_app_src(src/dfu_desc.c)
sdk_app_src(src/hpm_dfu_trigger.c)
sdk_app_src(src/boot_port_board_hpm.c)
sdk_app_src(src/dfu_flash_port.c)
sdk_app_src(src/vfat.c)
sdk_app_src(src/msc_if.c)

# CherryUSB DFU — SDK compiles usbd_dfu.c + hpm_dfu_port.c (DfuSe path)
# Keep include path for usbd_dfu.h / usb_dfu.h headers

# Compile definitions
sdk_compile_definitions(-DUSBD_DFU_APP_DEFAULT_ADD=0x80020000)

# Disable BOOT_PRINTF and UART console in Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    sdk_compile_definitions(-DCONFIG_NDEBUG_CONSOLE=1)
    message(STATUS "Release build: BOOT_PRINTF + console disabled")
endif()

message(STATUS "Board: ${BOARD}")

set(ELF ${CMAKE_BINARY_DIR}/output/${APP_NAME}.elf)
set(HEX ${CMAKE_BINARY_DIR}/output/${APP_NAME}.hex)

add_custom_command(OUTPUT ${HEX}
    COMMAND ${CMAKE_OBJCOPY} -O ihex -S ${ELF} ${HEX}
    DEPENDS ${ELF}
    COMMENT "Generating ${APP_NAME}.hex"
)
add_custom_target(artifacts ALL DEPENDS ${HEX})

# ---------------------------------------------------------------------------
# Post-build pack: append the bootloader info block (version, build time,
# hardware version, production date) at 0x8001F000 to the linker hex.
# ---------------------------------------------------------------------------
find_package(Python3 COMPONENTS Interpreter)
if(NOT Python3_EXECUTABLE)
    set(Python3_EXECUTABLE "$ENV{HPM_SDK_BASE}/../tools/python3/python.exe")
endif()

set(FW_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(APP_BIN ${CMAKE_BINARY_DIR}/output/${APP_NAME}.bin)
set(BL_PACK_HEX ${CMAKE_BINARY_DIR}/output/${APP_NAME}_pack.hex)

add_custom_command(OUTPUT ${BL_PACK_HEX}
    COMMAND ${Python3_EXECUTABLE} ${FW_ROOT}/tools/pack.py boot
            --hex ${HEX}
            --code-bin ${APP_BIN}
            --out-hex ${BL_PACK_HEX}
            --version-json ${FW_ROOT}/version.json
    DEPENDS ${HEX} ${FW_ROOT}/tools/pack.py ${FW_ROOT}/version.json
    COMMENT "Packing bootloader image (info block @0x8001F000)"
)
add_custom_target(pack_artifacts ALL DEPENDS ${BL_PACK_HEX})
