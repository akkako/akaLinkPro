/*
 * Copyright (c) 2013-2021 ARM Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ----------------------------------------------------------------------
 *
 * $Date:        16. June 2021
 * $Revision:    V2.1.0
 *
 * Project:      CMSIS-DAP Configuration
 * Title:        DAP_config.h CMSIS-DAP Configuration File (Template)
 *
 *---------------------------------------------------------------------------*/

#ifndef __DAP_CONFIG_H__
#define __DAP_CONFIG_H__

#include "hpm_common.h"
#include "pinmux.h"
#include "board.h"

//**************************************************************************************************
/**
\defgroup DAP_Config_Debug_gr CMSIS-DAP 调试探针单元信息
\ingroup DAP_ConfigIO_gr
@{
提供关于调试探针单元的硬件和配置的定义。

此信息包括：
 - 用于 CMSIS-DAP 调试探针单元的 Cortex-M 处理器参数定义。
 - 调试探针单元标识字符串（供应商、产品、序列号）。
 - 调试探针单元通信包大小。
 - 调试访问端口支持的模式和设置（JTAG/SWD 和 SWO）。
 - 关于已连接目标设备的可选信息（用于评估板）。
*/

#include "board.h"

#ifndef __STATIC_INLINE
#define __STATIC_INLINE static inline
#endif

#ifndef __STATIC_FORCEINLINE
#define __STATIC_FORCEINLINE static inline
#endif

// clang-format off

/// 调试探针单元中使用的 Cortex-M MCU 的处理器时钟。
/// 该值用于计算 SWD/JTAG 时钟速度。
#define CPU_CLOCK               360000000U      ///< 指定 CPU 时钟（单位：Hz）。

/// I/O 端口写操作所需的处理器周期数。
/// 该值用于计算在调试探针单元中由 Cortex-M MCU 通过 I/O 端口写操作生成的 SWD/JTAG 时钟速度。大多数 Cortex-M 处理器
/// 执行一次 I/O 端口写操作需要 2 个处理器周期。如果调试探针单元使用
/// 仅具有高速外设 I/O 的 Cortex-M0+ 处理器，则可能只需 1 个处理器周期。
#define IO_PORT_WRITE_CYCLES    3U              ///< I/O 周期：2=默认，1=Cortex-M0+ 快速 I/O。

/// 指示在调试访问端口上是否支持串行线调试 (SWD) 通信模式。
/// 该信息由命令 \ref DAP_Info 作为 <b>Capabilities</b> 的一部分返回。
#define DAP_SWD                 1               ///< SWD 模式：1 = 可用，0 = 不可用。

/// 指示在调试端口上是否支持 JTAG 通信模式。
/// 该信息由命令 \ref DAP_Info 作为 <b>Capabilities</b> 的一部分返回。
#define DAP_JTAG                1               ///< JTAG 模式：1 = 可用，0 = 不可用。

/// 配置连接到调试访问端口的扫描链上 JTAG 设备的最大数量。
/// 此设置影响调试探针单元的 RAM 需求。有效范围为 1 .. 255。
#define DAP_JTAG_DEV_CNT        8U              ///< 扫描链上 JTAG 设备的最大数量。

/// 调试访问端口上的默认通信模式。
/// 在选择端口默认模式时由命令 \ref DAP_Connect 使用。
#define DAP_DEFAULT_PORT        1U              ///< 默认 JTAG/SWJ 端口模式：1 = SWD，2 = JTAG。

/// 调试访问端口上 SWD 和 JTAG 模式的默认通信速度。
/// 用于初始化默认的 SWD/JTAG 时钟频率。
/// 命令 \ref DAP_SWJ_Clock 可用于覆盖此默认设置。
#define DAP_DEFAULT_SWJ_CLOCK   1000000U        ///< 默认 SWD/JTAG 时钟频率（单位：Hz）。

/// 命令和响应数据的最大包大小。
/// 此配置设置用于优化与调试器的通信性能，并取决于 USB 外设。典型值为：全速 USB HID 或 WinUSB 使用 64、
/// 高速 USB HID 使用 1024，高速 USB WinUSB 使用 512。
#define DAP_PACKET_SIZE         512U            ///< 指定包大小（单位：字节）。

/// 命令和响应数据的最大包缓冲区数量。
/// 此配置设置用于优化与调试器的通信性能，并取决于 USB 外设。对于 RAM 或 USB 缓冲区有限的设备，可以
/// 减小此设置（有效范围为 1 .. 255）。
#define DAP_PACKET_COUNT        4U              ///< 指定缓冲的包数量。

/// 指示是否支持 UART 串行线输出 (SWO) 跟踪。
/// 该信息由命令 \ref DAP_Info 作为 <b>Capabilities</b> 的一部分返回。
#define SWO_UART                0               ///< SWO UART：1 = 可用，0 = 不可用。

/// UART SWO 的 USART 驱动实例编号。
#define SWO_UART_DRIVER         0               ///< USART 驱动实例编号 (Driver_USART#)。

/// 最大 SWO UART 波特率。
#define SWO_UART_MAX_BAUDRATE   10000000U       ///< SWO UART 最大波特率（单位：Hz）。

/// 指示是否支持 Manchester 串行线输出 (SWO) 跟踪。
/// 该信息由命令 \ref DAP_Info 作为 <b>Capabilities</b> 的一部分返回。
#define SWO_MANCHESTER          0               ///< SWO Manchester：1 = 可用，0 = 不可用。

/// SWO 跟踪缓冲区大小。
#define SWO_BUFFER_SIZE         4096U           ///< SWO 跟踪缓冲区大小（单位：字节，必须为 2^n）。

/// SWO 流式跟踪。
#define SWO_STREAM              0               ///< SWO 流式跟踪：1 = 可用，0 = 不可用。

/// 测试域定时器的时钟频率。定时器值通过 \ref TIMESTAMP_GET 返回。
#define TIMESTAMP_CLOCK         0U      ///< 时间戳时钟（单位：Hz，0 = 不支持时间戳）。

/// 指示是否支持通过 USB COM 端口进行 UART 通信。
/// 该信息由命令 \ref DAP_Info 作为 <b>Capabilities</b> 的一部分返回。
#define DAP_UART_USB_COM_PORT   1               ///< USB COM 端口：1 = 可用，0 = 不可用。

// clang-format on

/** 获取供应商名称字符串。
\param str 指向用于存储字符串的缓冲区指针（最多 60 个字符）。
\return 字符串长度（包括终止的 NULL 字符）或 0（无字符串）。
*/
__STATIC_INLINE uint8_t DAP_GetVendorString(char *str)
{
    (void)str;
    return (0U);
}

/** 获取产品名称字符串。
\param str 指向用于存储字符串的缓冲区指针（最多 60 个字符）。
\return 字符串长度（包括终止的 NULL 字符）或 0（无字符串）。
*/
__STATIC_INLINE uint8_t DAP_GetProductString(char *str)
{
    (void)str;
    return (0U);
}

/** 获取序列号字符串。
\param str 指向用于存储字符串的缓冲区指针（最多 60 个字符）。
\return 字符串长度（包括终止的 NULL 字符）或 0（无字符串）。
*/
__STATIC_INLINE uint8_t DAP_GetSerNumString(char *str)
{
    (void)str;
    return (0U);
}

/** 获取目标设备供应商字符串。
\param str 指向用于存储字符串的缓冲区指针（最多 60 个字符）。
\return 字符串长度（包括终止的 NULL 字符）或 0（无字符串）。
*/
__STATIC_INLINE uint8_t DAP_GetTargetDeviceVendorString(char *str)
{
    (void)str;
    return (0U);
}

/** 获取目标设备名称字符串。
\param str 指向用于存储字符串的缓冲区指针（最多 60 个字符）。
\return 字符串长度（包括终止的 NULL 字符）或 0（无字符串）。
*/
__STATIC_INLINE uint8_t DAP_GetTargetDeviceNameString(char *str)
{
    (void)str;
    return (0U);
}

/** 获取目标板卡供应商字符串。
\param str 指向用于存储字符串的缓冲区指针（最多 60 个字符）。
\return 字符串长度（包括终止的 NULL 字符）或 0（无字符串）。
*/
__STATIC_INLINE uint8_t DAP_GetTargetBoardVendorString(char *str)
{
    (void)str;
    return (0U);
}

/** 获取目标板卡名称字符串。
\param str 指向用于存储字符串的缓冲区指针（最多 60 个字符）。
\return 字符串长度（包括终止的 NULL 字符）或 0（无字符串）。
*/
__STATIC_INLINE uint8_t DAP_GetTargetBoardNameString(char *str)
{
    (void)str;
    return (0U);
}

/** 获取产品固件版本字符串。
\param str 指向用于存储字符串的缓冲区指针（最多 60 个字符）。
\return 字符串长度（包括终止的 NULL 字符）或 0（无字符串）。
*/
__STATIC_INLINE uint8_t DAP_GetProductFirmwareVersionString(char *str)
{
    (void)str;
    return (0U);
}

#include "hpm_gpio_drv.h"
#include "hpm_gpiom_drv.h"

__STATIC_INLINE void gpiom_config_pin_to_fgpio(uint16_t gpio_index)
{
    gpiom_set_pin_controller(PIN_GPIOM_BASE,
                             GPIO_GET_PORT_INDEX(gpio_index),
                             GPIO_GET_PIN_INDEX(gpio_index),
                             PIN_GPIOM);
    gpiom_enable_pin_visibility(PIN_GPIOM_BASE,
                                GPIO_GET_PORT_INDEX(gpio_index),
                                GPIO_GET_PIN_INDEX(gpio_index),
                                PIN_GPIOM);
}

///@}

//**************************************************************************************************
/**
\defgroup DAP_Config_PortIO_gr CMSIS-DAP 硬件 I/O 引脚访问
\ingroup DAP_ConfigIO_gr
@{

CMSIS-DAP 硬件调试端口的标准 I/O 引脚支持标准 JTAG 模式
和串行线调试 (SWD) 模式。在 SWD 模式下，实现设备的调试
接口仅需 2 个引脚。提供以下 I/O 引脚：

JTAG I/O 引脚                 | SWD I/O 引脚         | CMSIS-DAP 硬件引脚模式
---------------------------- | -------------------- | ---------------------------------------------
TCK: 测试时钟                | SWCLK: 时钟          | 输出 推挽
TMS: 测试模式选择            | SWDIO: 数据 I/O      | 输出 推挽；输入（用于接收数据）
TDI: 测试数据输入            |                      | 输出 推挽
TDO: 测试数据输出            |                      | 输入
nTRST: 测试复位（可选）      |                      | 输出 开漏带上拉电阻
nRESET: 设备复位             | nRESET: 设备复位     | 输出 开漏带上拉电阻


DAP 硬件 I/O 引脚访问函数
-------------------------------------
各种 I/O 引脚通过实现读取、写入、置位或清零操作的
函数来访问。

对于 SWDIO I/O 引脚，仅在 SWD I/O 模式下调用的额外函数。
提供这些函数是为了在使用某些高级 GPIO 外设时实现更快的 I/O，
这些外设可以独立地写入/读取单个 I/O 引脚而不影响同一 I/O 端口的
其他引脚。提供以下 SWDIO I/O 引脚函数：
 - \ref PIN_SWDIO_OUT_ENABLE 使能 DAP 硬件的输出模式。
 - \ref PIN_SWDIO_OUT_DISABLE 使能 DAP 硬件的输入模式。
 - \ref PIN_SWDIO_IN 以尽可能高的速度从 SWDIO I/O 引脚读取。
 - \ref PIN_SWDIO_OUT 以尽可能高的速度向 SWDIO I/O 引脚写入。
*/

// 配置 DAP I/O 引脚 ------------------------------

/** 设置 JTAG I/O 引脚：TCK、TMS、TDI、TDO、nTRST 和 nRESET。
配置用于 JTAG 模式的 DAP 硬件 I/O 引脚：
 - TCK、TMS、TDI、nTRST、nRESET 设置为输出模式并置为高电平。
 - TDO 设置为输入模式。
*/
__STATIC_INLINE void PORT_JTAG_SETUP(void)
{
    // 设置 IO 由 FGPIO 驱动
    gpiom_config_pin_to_fgpio(BOARD_PIN_JTCK);
    gpiom_config_pin_to_fgpio(BOARD_PIN_JTMS_IN);
    gpiom_config_pin_to_fgpio(BOARD_PIN_JTMS_OUT);
    gpiom_config_pin_to_fgpio(BOARD_PIN_JTMS_DIR);
    gpiom_config_pin_to_fgpio(BOARD_PIN_nRESET);
    gpiom_config_pin_to_fgpio(BOARD_PIN_JTRST);
    gpiom_config_pin_to_fgpio(BOARD_PIN_JTDO);
    gpiom_config_pin_to_fgpio(BOARD_PIN_JTDI);

    // 设置输入输出模式
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTCK), GPIO_GET_PIN_INDEX(BOARD_PIN_JTCK));
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTMS_OUT), GPIO_GET_PIN_INDEX(BOARD_PIN_JTMS_OUT));
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTMS_DIR), GPIO_GET_PIN_INDEX(BOARD_PIN_JTMS_DIR));
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_nRESET), GPIO_GET_PIN_INDEX(BOARD_PIN_nRESET));
    gpio_set_pin_input(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTMS_IN), GPIO_GET_PIN_INDEX(BOARD_PIN_JTMS_IN));
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTRST), GPIO_GET_PIN_INDEX(BOARD_PIN_JTRST));
    gpio_set_pin_input(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTDO), GPIO_GET_PIN_INDEX(BOARD_PIN_JTDO));
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTDI), GPIO_GET_PIN_INDEX(BOARD_PIN_JTDI));

    // 设置默认输出电平
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTCK), GPIO_GET_PIN_INDEX(BOARD_PIN_JTCK), 1);
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTMS_DIR), GPIO_GET_PIN_INDEX(BOARD_PIN_JTMS_DIR), 1); // output
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTMS_OUT), GPIO_GET_PIN_INDEX(BOARD_PIN_JTMS_OUT), 1);
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_nRESET), GPIO_GET_PIN_INDEX(BOARD_PIN_nRESET), 0);
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTRST), GPIO_GET_PIN_INDEX(BOARD_PIN_JTRST), 1);
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTDI), GPIO_GET_PIN_INDEX(BOARD_PIN_JTDI), 1);
}

/** 设置 SWD I/O 引脚：SWCLK、SWDIO 和 nRESET。
配置用于串行线调试 (SWD) 模式的 DAP 硬件 I/O 引脚：
 - SWCLK、SWDIO、nRESET 设置为输出模式并置为默认高电平。
 - TDI、nTRST 设置为高阻模式（在 SWD 模式下未使用这些引脚）。
*/
__STATIC_INLINE void PORT_SWD_SETUP(void)
{
    // 设置 IO 由 FGPIO 驱动
    gpiom_config_pin_to_fgpio(BOARD_PIN_JTCK);
    gpiom_config_pin_to_fgpio(BOARD_PIN_JTMS_IN);
    gpiom_config_pin_to_fgpio(BOARD_PIN_JTMS_OUT);
    gpiom_config_pin_to_fgpio(BOARD_PIN_JTMS_DIR);
    gpiom_config_pin_to_fgpio(BOARD_PIN_nRESET);
    gpiom_config_pin_to_fgpio(BOARD_PIN_JTRST);
    gpiom_config_pin_to_fgpio(BOARD_PIN_JTDO);
    gpiom_config_pin_to_fgpio(BOARD_PIN_JTDI);

    // 设置输入输出模式
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTCK), GPIO_GET_PIN_INDEX(BOARD_PIN_JTCK));
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTMS_OUT), GPIO_GET_PIN_INDEX(BOARD_PIN_JTMS_OUT));
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTMS_DIR), GPIO_GET_PIN_INDEX(BOARD_PIN_JTMS_DIR));
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_nRESET), GPIO_GET_PIN_INDEX(BOARD_PIN_nRESET));
    gpio_set_pin_input(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTMS_IN), GPIO_GET_PIN_INDEX(BOARD_PIN_JTMS_IN));
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTRST), GPIO_GET_PIN_INDEX(BOARD_PIN_JTRST));
    gpio_set_pin_input(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTDO), GPIO_GET_PIN_INDEX(BOARD_PIN_JTDO));
    gpio_set_pin_output(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTDI), GPIO_GET_PIN_INDEX(BOARD_PIN_JTDI));

    // 设置默认输出电平
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTCK), GPIO_GET_PIN_INDEX(BOARD_PIN_JTCK), 1);
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTMS_DIR), GPIO_GET_PIN_INDEX(BOARD_PIN_JTMS_DIR), 1);
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTMS_OUT), GPIO_GET_PIN_INDEX(BOARD_PIN_JTMS_OUT), 1);
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_nRESET), GPIO_GET_PIN_INDEX(BOARD_PIN_nRESET), 0);
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTRST), GPIO_GET_PIN_INDEX(BOARD_PIN_JTRST), 1);
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTDI), GPIO_GET_PIN_INDEX(BOARD_PIN_JTDI), 1);
}

/** 禁用 JTAG/SWD I/O 引脚。
禁用 DAP 硬件 I/O 引脚，配置如下：
 - TCK/SWCLK、TMS/SWDIO、TDI、TDO、nTRST、nRESET 设置为高阻模式。
*/
__STATIC_INLINE void PORT_OFF(void)
{
    // 设置默认输出电平
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTCK), GPIO_GET_PIN_INDEX(BOARD_PIN_JTCK), 1);
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTMS_DIR), GPIO_GET_PIN_INDEX(BOARD_PIN_JTMS_DIR), 1);
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTMS_OUT), GPIO_GET_PIN_INDEX(BOARD_PIN_JTMS_OUT), 1);
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_nRESET), GPIO_GET_PIN_INDEX(BOARD_PIN_nRESET), 0);
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTRST), GPIO_GET_PIN_INDEX(BOARD_PIN_JTRST), 1);
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTDI), GPIO_GET_PIN_INDEX(BOARD_PIN_JTDI), 1);
}

// SWCLK/TCK I/O 引脚 -------------------------------------

/** SWCLK/TCK I/O 引脚：获取输入。
\return SWCLK/TCK DAP 硬件 I/O 引脚的当前状态。
*/
__STATIC_FORCEINLINE uint32_t PIN_SWCLK_TCK_IN(void)
{
    return (gpio_get_pin_output_status(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTCK), GPIO_GET_PIN_INDEX(BOARD_PIN_JTCK)));
}

/** SWCLK/TCK I/O 引脚：设置输出为高电平。
将 SWCLK/TCK DAP 硬件 I/O 引脚设置为高电平。
*/
__STATIC_FORCEINLINE void PIN_SWCLK_TCK_SET(void)
{
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTCK), GPIO_GET_PIN_INDEX(BOARD_PIN_JTCK), 1);
}

/** SWCLK/TCK I/O 引脚：设置输出为低电平。
将 SWCLK/TCK DAP 硬件 I/O 引脚设置为低电平。
*/
__STATIC_FORCEINLINE void PIN_SWCLK_TCK_CLR(void)
{
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTCK), GPIO_GET_PIN_INDEX(BOARD_PIN_JTCK), 0);
}

// SWDIO/TMS 引脚 I/O --------------------------------------

/** SWDIO/TMS I/O 引脚：获取输入。
\return SWDIO/TMS DAP 硬件 I/O 引脚的当前状态。
*/
__STATIC_FORCEINLINE uint32_t PIN_SWDIO_TMS_IN(void)
{
    return (gpio_read_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTMS_IN), GPIO_GET_PIN_INDEX(BOARD_PIN_JTMS_IN)));
}

/** SWDIO/TMS I/O 引脚：设置输出为高电平。
将 SWDIO/TMS DAP 硬件 I/O 引脚设置为高电平。
*/
__STATIC_FORCEINLINE void PIN_SWDIO_TMS_SET(void)
{
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTMS_OUT), GPIO_GET_PIN_INDEX(BOARD_PIN_JTMS_OUT), 1);
}

/** SWDIO/TMS I/O 引脚：设置输出为低电平。
将 SWDIO/TMS DAP 硬件 I/O 引脚设置为低电平。
*/
__STATIC_FORCEINLINE void PIN_SWDIO_TMS_CLR(void)
{
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTMS_OUT), GPIO_GET_PIN_INDEX(BOARD_PIN_JTMS_OUT), 0);
}

/** SWDIO I/O 引脚：获取输入（仅在 SWD 模式下使用）。
\return SWDIO DAP 硬件 I/O 引脚的当前状态。
*/
__STATIC_FORCEINLINE uint32_t PIN_SWDIO_IN(void)
{
    return (gpio_read_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTMS_IN), GPIO_GET_PIN_INDEX(BOARD_PIN_JTMS_IN)));
}

/** SWDIO I/O 引脚：设置输出（仅在 SWD 模式下使用）。
\param bit SWDIO DAP 硬件 I/O 引脚的输出值。
*/
__STATIC_FORCEINLINE void PIN_SWDIO_OUT(uint32_t bit)
{
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTMS_OUT), GPIO_GET_PIN_INDEX(BOARD_PIN_JTMS_OUT), bit & 0x01);
}

/** SWDIO I/O 引脚：切换到输出模式（仅在 SWD 模式下使用）。
配置 SWDIO DAP 硬件 I/O 引脚为输出模式。此函数在
调用 \ref PIN_SWDIO_OUT 函数之前被调用。
*/
__STATIC_FORCEINLINE void PIN_SWDIO_OUT_ENABLE(void)
{
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTMS_DIR), GPIO_GET_PIN_INDEX(BOARD_PIN_JTMS_DIR), 1);
}

/** SWDIO I/O 引脚：切换到输入模式（仅在 SWD 模式下使用）。
配置 SWDIO DAP 硬件 I/O 引脚为输入模式。此函数在
调用 \ref PIN_SWDIO_IN 函数之前被调用。
*/
__STATIC_FORCEINLINE void PIN_SWDIO_OUT_DISABLE(void)
{
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTMS_DIR), GPIO_GET_PIN_INDEX(BOARD_PIN_JTMS_DIR), 0);
}

// TDI 引脚 I/O ---------------------------------------------

/** TDI I/O 引脚：获取输入。
\return TDI DAP 硬件 I/O 引脚的当前状态。
*/
__STATIC_FORCEINLINE uint32_t PIN_TDI_IN(void)
{
    return (gpio_get_pin_output_status(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTDI), GPIO_GET_PIN_INDEX(BOARD_PIN_JTDI)));
}

/** TDI I/O 引脚：设置输出。
\param bit TDI DAP 硬件 I/O 引脚的输出值。
*/
__STATIC_FORCEINLINE void PIN_TDI_OUT(uint32_t bit)
{
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTDI), GPIO_GET_PIN_INDEX(BOARD_PIN_JTDI), bit & 0x01);
}

// TDO 引脚 I/O ---------------------------------------------

/** TDO I/O 引脚：获取输入。
\return TDO DAP 硬件 I/O 引脚的当前状态。
*/
__STATIC_FORCEINLINE uint32_t PIN_TDO_IN(void)
{
    return (gpio_read_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTDO), GPIO_GET_PIN_INDEX(BOARD_PIN_JTDO)));
}

// nTRST 引脚 I/O -------------------------------------------

/** nTRST I/O 引脚：获取输入。
\return nTRST DAP 硬件 I/O 引脚的当前状态。
*/
__STATIC_FORCEINLINE uint32_t PIN_nTRST_IN(void)
{
    return (gpio_get_pin_output_status(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTRST), GPIO_GET_PIN_INDEX(BOARD_PIN_JTRST)));
}

/** nTRST I/O 引脚：设置输出。
\param bit JTAG TRST 测试复位引脚状态：
           - 0：发起 JTAG TRST 测试复位。
           - 1：释放 JTAG TRST 测试复位。
*/
__STATIC_FORCEINLINE void PIN_nTRST_OUT(uint32_t bit)
{
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_JTRST), GPIO_GET_PIN_INDEX(BOARD_PIN_JTRST), (bit & 0x01));
}

// nRESET 引脚 I/O ------------------------------------------

/** nRESET I/O 引脚：获取输入。
\return nRESET DAP 硬件 I/O 引脚的当前状态。
*/
__STATIC_FORCEINLINE uint32_t PIN_nRESET_IN(void)
{
    return (!gpio_get_pin_output_status(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_nRESET), GPIO_GET_PIN_INDEX(BOARD_PIN_nRESET)));
}

/** nRESET I/O 引脚：设置输出。
\param bit 目标设备硬件复位引脚状态：
           - 0：发起设备硬件复位。
           - 1：释放设备硬件复位。
*/
__STATIC_FORCEINLINE void PIN_nRESET_OUT(uint32_t bit)
{
    gpio_write_pin(PIN_GPIO, GPIO_GET_PORT_INDEX(BOARD_PIN_nRESET), GPIO_GET_PIN_INDEX(BOARD_PIN_nRESET), !bit);
}

///@}

//**************************************************************************************************
/**
\defgroup DAP_Config_LEDs_gr CMSIS-DAP 硬件状态 LED
\ingroup DAP_ConfigIO_gr
@{

CMSIS-DAP 硬件可提供指示 CMSIS-DAP 调试探针单元状态的 LED。

建议提供以下 LED 用于状态指示：
 - 连接 LED：当 DAP 硬件连接到调试器时激活。
 - 运行 LED：当调试器使目标设备进入运行状态时激活。
*/

/** 调试探针单元：设置已连接 LED 的状态。
\param bit 连接 LED 的状态。
           - 1：连接 LED 亮：调试器已连接到 CMSIS-DAP 调试探针单元。
           - 0：连接 LED 灭：调试器未连接到 CMSIS-DAP 调试探针单元。
*/
__STATIC_INLINE void LED_CONNECTED_OUT(uint32_t bit) {}

/** 调试探针单元：设置目标运行 LED 的状态。
\param bit 目标运行 LED 的状态。
           - 1：目标运行 LED 亮：目标中的程序执行已启动。
           - 0：目标运行 LED 灭：目标中的程序执行已停止。
*/
__STATIC_INLINE void LED_RUNNING_OUT(uint32_t bit) {}

///@}

//**************************************************************************************************
/**
\defgroup DAP_Config_Timestamp_gr CMSIS-DAP 时间戳
\ingroup DAP_ConfigIO_gr
@{
访问测试域定时器的函数。

调试探针单元中测试域定时器的值通过函数 \ref TIMESTAMP_GET 返回。默认情况下，
使用 DWT 定时器。该定时器的频率通过 \ref TIMESTAMP_CLOCK 配置。

*/

/** 获取测试域定时器的时间戳。
\return 当前时间戳值。
*/
__STATIC_INLINE uint32_t TIMESTAMP_GET(void)
{
    // return (DWT->CYCCNT);
    return (0U);
}

///@}

//**************************************************************************************************
/**
\defgroup DAP_Config_Initialization_gr CMSIS-DAP 初始化
\ingroup DAP_ConfigIO_gr
@{

CMSIS-DAP 硬件 I/O 和 LED 引脚通过函数 \ref DAP_SETUP 进行初始化。
*/

/** 设置调试探针单元的 I/O 引脚和 LED（在调试探针单元初始化时调用）。
此函数执行 CMSIS-DAP 硬件 I/O 引脚和
状态 LED 的初始化。具体而言，硬件 I/O 和 LED 引脚的操作被使能并设置如下：
 - 使能 I/O 时钟系统。
 - 所有 I/O 引脚：使能输入缓冲区，输出引脚设置为高阻模式。
 - 对于 nTRST、nRESET，使能弱上拉（如果可用）。
 - 使能 LED 输出引脚并关闭 LED。
*/
__STATIC_INLINE void DAP_SETUP(void)
{
    // 配置 IOC->PAD[FUNC_CTL] 为 GPIO
    HPM_IOC->PAD[BOARD_PIN_JTCK].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0) | IOC_PAD_FUNC_CTL_LOOP_BACK_MASK;
    HPM_IOC->PAD[BOARD_PIN_JTMS_IN].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[BOARD_PIN_JTMS_OUT].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[BOARD_PIN_JTMS_DIR].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[BOARD_PIN_nRESET].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[BOARD_PIN_JTDO].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[BOARD_PIN_JTDI].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[BOARD_PIN_JTRST].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);

    // 配置 IOC->PAD[PAD_CTL]
    HPM_IOC->PAD[BOARD_PIN_JTCK].PAD_CTL =
        IOC_PAD_PAD_CTL_HYS_SET(0) | // 施密特触发器 失效
        IOC_PAD_PAD_CTL_PRS_SET(0) | // 上下拉强度 100k
        IOC_PAD_PAD_CTL_PS_SET(1) |  // 上拉
        IOC_PAD_PAD_CTL_PE_SET(1) |  // 上拉使能
        IOC_PAD_PAD_CTL_KE_SET(0) |  // 保持能力 失效
        IOC_PAD_PAD_CTL_OD_SET(0) |  // 开漏输出 失效
        IOC_PAD_PAD_CTL_SR_SET(1) |  // 压摆率 快速
        IOC_PAD_PAD_CTL_SPD_SET(3) | // 最快压摆率
        IOC_PAD_PAD_CTL_DS_SET(4);   // 驱动能力 39 ohm(3.3V)
    HPM_IOC->PAD[BOARD_PIN_JTMS_IN].PAD_CTL =
        IOC_PAD_PAD_CTL_HYS_SET(0) | // 施密特触发器 失效
        IOC_PAD_PAD_CTL_PRS_SET(0) | // 上下拉强度 100k
        IOC_PAD_PAD_CTL_PS_SET(1) |  // 上拉
        IOC_PAD_PAD_CTL_PE_SET(1) |  // 上拉使能
        IOC_PAD_PAD_CTL_KE_SET(0) |  // 保持能力 失效
        IOC_PAD_PAD_CTL_OD_SET(0) |  // 开漏输出 失效
        IOC_PAD_PAD_CTL_SR_SET(1) |  // 压摆率 快速
        IOC_PAD_PAD_CTL_SPD_SET(3) | // 最快压摆率
        IOC_PAD_PAD_CTL_DS_SET(4);   // 驱动能力 39 ohm(3.3V)
    HPM_IOC->PAD[BOARD_PIN_JTMS_OUT].PAD_CTL =
        IOC_PAD_PAD_CTL_HYS_SET(0) | // 施密特触发器 失效
        IOC_PAD_PAD_CTL_PRS_SET(0) | // 上下拉强度 100k
        IOC_PAD_PAD_CTL_PS_SET(1) |  // 上拉
        IOC_PAD_PAD_CTL_PE_SET(1) |  // 上拉使能
        IOC_PAD_PAD_CTL_KE_SET(0) |  // 保持能力 失效
        IOC_PAD_PAD_CTL_OD_SET(0) |  // 开漏输出 失效
        IOC_PAD_PAD_CTL_SR_SET(1) |  // 压摆率 快速
        IOC_PAD_PAD_CTL_SPD_SET(3) | // 最快压摆率
        IOC_PAD_PAD_CTL_DS_SET(4);   // 驱动能力 39 ohm(3.3V)
    HPM_IOC->PAD[BOARD_PIN_JTMS_DIR].PAD_CTL =
        IOC_PAD_PAD_CTL_HYS_SET(0) | // 施密特触发器 失效
        IOC_PAD_PAD_CTL_PRS_SET(0) | // 上下拉强度 100k
        IOC_PAD_PAD_CTL_PS_SET(1) |  // 上拉
        IOC_PAD_PAD_CTL_PE_SET(1) |  // 上拉使能
        IOC_PAD_PAD_CTL_KE_SET(0) |  // 保持能力 失效
        IOC_PAD_PAD_CTL_OD_SET(0) |  // 开漏输出 失效
        IOC_PAD_PAD_CTL_SR_SET(1) |  // 压摆率 快速
        IOC_PAD_PAD_CTL_SPD_SET(3) | // 最快压摆率
        IOC_PAD_PAD_CTL_DS_SET(4);   // 驱动能力 39 ohm(3.3V)
    HPM_IOC->PAD[BOARD_PIN_nRESET].PAD_CTL =
        IOC_PAD_PAD_CTL_HYS_SET(0) | // 施密特触发器 失效
        IOC_PAD_PAD_CTL_PRS_SET(0) | // 上下拉强度 100k
        IOC_PAD_PAD_CTL_PS_SET(0) |  // 下拉
        IOC_PAD_PAD_CTL_PE_SET(1) |  // 下拉使能
        IOC_PAD_PAD_CTL_KE_SET(0) |  // 保持能力 失效
        IOC_PAD_PAD_CTL_OD_SET(0) |  // 开漏输出 失效
        IOC_PAD_PAD_CTL_SR_SET(0) |  // 压摆率 慢速
        IOC_PAD_PAD_CTL_SPD_SET(1) | // 较慢压摆率
        IOC_PAD_PAD_CTL_DS_SET(4);   // 驱动能力 39 ohm(3.3V)
    HPM_IOC->PAD[BOARD_PIN_JTDO].PAD_CTL =
        IOC_PAD_PAD_CTL_HYS_SET(0) | // 施密特触发器 失效
        IOC_PAD_PAD_CTL_PRS_SET(0) | // 上下拉强度 100k
        IOC_PAD_PAD_CTL_PS_SET(1) |  // 上拉
        IOC_PAD_PAD_CTL_PE_SET(1) |  // 上拉使能
        IOC_PAD_PAD_CTL_KE_SET(0) |  // 保持能力 失效
        IOC_PAD_PAD_CTL_OD_SET(0) |  // 开漏输出 失效
        IOC_PAD_PAD_CTL_SR_SET(1) |  // 压摆率 快速
        IOC_PAD_PAD_CTL_SPD_SET(3) | // 最快压摆率
        IOC_PAD_PAD_CTL_DS_SET(4);   // 驱动能力 39 ohm(3.3V)
    HPM_IOC->PAD[BOARD_PIN_JTDI].PAD_CTL =
        IOC_PAD_PAD_CTL_HYS_SET(0) | // 施密特触发器 失效
        IOC_PAD_PAD_CTL_PRS_SET(0) | // 上下拉强度 100k
        IOC_PAD_PAD_CTL_PS_SET(1) |  // 上拉
        IOC_PAD_PAD_CTL_PE_SET(1) |  // 上拉使能
        IOC_PAD_PAD_CTL_KE_SET(0) |  // 保持能力 失效
        IOC_PAD_PAD_CTL_OD_SET(0) |  // 开漏输出 失效
        IOC_PAD_PAD_CTL_SR_SET(1) |  // 压摆率 快速
        IOC_PAD_PAD_CTL_SPD_SET(3) | // 最快压摆率
        IOC_PAD_PAD_CTL_DS_SET(4);   // 驱动能力 39 ohm(3.3V)
    HPM_IOC->PAD[BOARD_PIN_JTRST].PAD_CTL =
        IOC_PAD_PAD_CTL_HYS_SET(0) | // 施密特触发器 失效
        IOC_PAD_PAD_CTL_PRS_SET(0) | // 上下拉强度 100k
        IOC_PAD_PAD_CTL_PS_SET(1) |  // 上拉
        IOC_PAD_PAD_CTL_PE_SET(1) |  // 上拉使能
        IOC_PAD_PAD_CTL_KE_SET(0) |  // 保持能力 失效
        IOC_PAD_PAD_CTL_OD_SET(0) |  // 开漏输出 失效
        IOC_PAD_PAD_CTL_SR_SET(1) |  // 压摆率 快速
        IOC_PAD_PAD_CTL_SPD_SET(3) | // 最快压摆率
        IOC_PAD_PAD_CTL_DS_SET(4);   // 驱动能力 39 ohm(3.3V)
}

/** 使用自定义特定的 I/O 引脚或命令序列复位目标设备。
此函数允许选择性地实现设备特定的复位序列。
它在调用命令 \ref DAP_ResetTarget 时被调用，例如当
设备需要启用调试端口的时效性解锁序列时需要此函数。
\return 0 = 未实现设备特定的复位序列。\n
        1 = 已实现设备特定的复位序列。
*/
__STATIC_INLINE uint8_t RESET_TARGET(void)
{
    return (0U); // 当实现了设备复位序列时改为 '1'
}

///@}

#endif /* __DAP_CONFIG_H__ */
