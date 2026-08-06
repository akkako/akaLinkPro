#ifndef __DAP_PORT_H__
#define __DAP_PORT_H__

#include "DAP_config.h"

#include "ch32v30x_gpio.h"
#include "ch32v30x_rcc.h"
#include "drv_spi.h"
#include "drv_usb2uart.h"

/**
 * @brief DAP 获取厂商字符串
 *
 * @param str 字符串指针
 * @return 字符串长度
 */
__STATIC_INLINE uint8_t DAP_GetVendorString(char *str)
{
    str = DAP_STR_VENDOR_NAME;
    return sizeof(DAP_STR_VENDOR_NAME);
}

/**
 * @brief DAP 获取产品名称字符串
 *
 * @param str 字符串指针
 * @return 字符串长度
 */
__STATIC_INLINE uint8_t DAP_GetProductString(char *str)
{
    str = DAP_STR_PRODUCT_NAME;
    return sizeof(DAP_STR_PRODUCT_NAME);
}

/**
 * @brief DAP 获取序列号字符串
 *
 * @param str 字符串指针
 * @return 字符串长度
 */
__STATIC_INLINE uint8_t DAP_GetSerNumString(char *str)
{
    (void)str;
    return (0U);
}

/**
 * @brief DAP 获取固件版本字符串
 *
 * @param str 字符串指针
 * @return 字符串长度
 */
__STATIC_INLINE uint8_t DAP_GetProductFirmwareVersionString(char *str)
{
    str = DAP_STR_FIRMWARE_VER;
    return sizeof(DAP_STR_FIRMWARE_VER);
}

/*
| Name     | Pin      | JTAG       | SWD        | Alternate |
| -------- | -------- | ---------- | ---------- | --------- |
| ADCV     | PA1      |            |            | ADC_CH1   |
| TRST     | PC6      | JTAG_TRST  | N/A(INPUT) | GPIO      |
| LED      | PC7      |            |            | GPIO      |
| SRST     | PC8      |            |            | GPIO      |
| 5VEN     | PC9      |            |            | GPIO      |
| TDI      | PB10     | JTAG_TDI   | UART_TXD   | UART3_TXD |
| TDO      | PB11     | JTAG_TDO   | UART_RXD   | UART3_RXD |
| SWDIR    | PB12     | N/A(HIGH)  | SWDIR      | GPIO      |
| TCK      | PB13     | JTAG_TCK   | SWCLK      | SPI2_SCLK |
| TMSI     | PB14     | N/A(INPUT) | SWDI       | SPI2_MISO |
| TMSO     | PB15     | JTAG_TMS   | SWDO       | SPI2_MOSI |
*/

#define SWD_GPIO GPIOB
#define JTAG_GPIO GPIOB
#define JTRST_GPIO GPIOC
#define SRST_GPIO GPIOC

#define SWDIR_PIN GPIO_Pin_12
#define SWDI_PIN GPIO_Pin_14
#define SWDO_PIN GPIO_Pin_15
#define SWCK_PIN GPIO_Pin_13

#define JTCK_PIN SWCK_PIN
#define JTMS_PIN SWDO_PIN
#define JTDI_PIN GPIO_Pin_10
#define JTDO_PIN GPIO_Pin_11
#define JTRST_PIN GPIO_Pin_6

#define SRST_PIN GPIO_Pin_8

#define SWCK_BIT_PIN (13 - 8)
#define SWDI_BIT_PIN (14 - 8)
#define SWDO_BIT_PIN (15 - 8)

/**
 * @brief DAP 引脚初始化
 * @note 初始化 GPIO 时钟
 *       初始化输入引脚为输入状态，输出引脚为高阻状态
 *       初始化复位输出引脚为上拉状态
 *       初始化 LED 状态
 *
 * @return None
 */
__STATIC_INLINE void DAP_SETUP(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
    GPIO_Init(GPIOC, &GPIO_InitStruct);
    GPIO_ResetBits(GPIOC, GPIO_Pin_8 | GPIO_Pin_9);

    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Pin = SWDIR_PIN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SWD_GPIO, &GPIO_InitStruct);
    GPIO_ResetBits(SWD_GPIO, SWDIR_PIN);

    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStruct.GPIO_Pin = SWCK_PIN | SWDO_PIN | JTDI_PIN | JTDO_PIN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SWD_GPIO, &GPIO_InitStruct);
    GPIO_ResetBits(SWD_GPIO, SWDIR_PIN);

    GPIO_InitStruct.GPIO_Pin = JTRST_PIN;
    GPIO_Init(JTRST_GPIO, &GPIO_InitStruct);

    drv_usb2uart_gpio_af_uart();
}

/**
 * @brief 获取当前时间戳
 *
 * @return 时间戳
 */
__STATIC_INLINE uint32_t TIMESTAMP_GET(void)
{
    // return (DWT->CYCCNT);
    return 0;
}

/** DAP 引脚操作函数移植 **/

/** 初始化配置 JTAG 引脚，并配置默认输出电平
    TCK    --> 推挽输出/高电平
    TMS    --> 推挽输出/高电平
    TDI    --> 推挽输出/高电平
    TDO    --> 上拉输入
    nTRST  --> 推挽输出/高电平
    nRESET --> 开漏输出/高电平
*/
__STATIC_INLINE void PORT_JTAG_SETUP(void)
{
    GPIO_SetBits(SWD_GPIO, SWDIR_PIN);

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Pin = JTCK_PIN | JTMS_PIN | JTDI_PIN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(JTAG_GPIO, &GPIO_InitStruct);
    GPIO_SetBits(JTAG_GPIO, JTCK_PIN | JTMS_PIN | JTDI_PIN);

    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Pin = JTRST_PIN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(JTRST_GPIO, &GPIO_InitStruct);
    GPIO_SetBits(JTRST_GPIO, JTRST_PIN);

    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStruct.GPIO_Pin = SWDI_PIN | JTDO_PIN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SWD_GPIO, &GPIO_InitStruct);

    // drv_usb2uart_gpio_af_uart();
}

/** 初始化配置 SWD 引脚，并配置默认输出电平
    TCK    --> 推挽输出/高电平
    TMS    --> 推挽输出/高电平
    TDI(TX)--> 复用输出
    TDO(RX)--> 复用输入
    nTRST  --> 高阻
    nRESET --> 开漏输出/高电平
*/
__STATIC_INLINE void PORT_SWD_SETUP(void)
{
    GPIO_SetBits(SWD_GPIO, SWCK_PIN | SWDO_PIN);
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Pin = SWCK_PIN | SWDO_PIN | SWDIR_PIN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SWD_GPIO, &GPIO_InitStruct);
    GPIO_SetBits(SWD_GPIO, SWCK_PIN | SWDO_PIN | SWDIR_PIN);

    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStruct.GPIO_Pin = SWDI_PIN | JTDI_PIN | JTDO_PIN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SWD_GPIO, &GPIO_InitStruct);
}

/** 禁用调试接口引脚输出
    TCK    --> 高阻
    TMS    --> 高阻
    TDI    --> 高阻
    TDO    --> 高阻
    nTRST  --> 高阻
    nRESET --> 高阻
*/
__STATIC_INLINE void PORT_OFF(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStruct.GPIO_Pin = SWCK_PIN | SWDO_PIN | JTDI_PIN | JTDO_PIN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SWD_GPIO, &GPIO_InitStruct);
    GPIO_ResetBits(SWD_GPIO, SWDIR_PIN);

    GPIO_InitStruct.GPIO_Pin = JTRST_PIN;
    GPIO_Init(JTRST_GPIO, &GPIO_InitStruct);

    drv_usb2uart_gpio_af_uart();
}

// SWCLK/TCK 引脚 -------------------------------------

/**
 * @brief 读取 SWCLK/TCK 引脚当前输出电平
 *
 * @return 电平状态
 */
__STATIC_FORCEINLINE uint32_t PIN_SWCLK_TCK_IN(void)
{
    // return GPIO_ReadOutputDataBit(SWD_GPIO, SWCK_PIN);
    return SWD_GPIO->OUTDR & SWCK_PIN ? 1 : 0;
}

/**
 * @brief 设置 SWCLK/TCK 引脚输出电平为高
 *
 * @return None
 */
__STATIC_FORCEINLINE void PIN_SWCLK_TCK_SET(void)
{
    GPIO_SetBits(SWD_GPIO, SWCK_PIN);
}

/**
 * @brief 设置 SWCLK/TCK 引脚输出电平为低
 *
 * @return None
 */
__STATIC_FORCEINLINE void PIN_SWCLK_TCK_CLR(void)
{
    GPIO_ResetBits(SWD_GPIO, SWCK_PIN);
}

// SWDIO/TMS 引脚 --------------------------------------

/**
 * @brief 读取 SWDIO/TMS 引脚当前输入电平
 *
 * @return 电平状态
 */
__STATIC_FORCEINLINE uint32_t PIN_SWDIO_TMS_IN(void)
{
    return GPIO_ReadInputDataBit(SWD_GPIO, SWDI_PIN);
    // return SWD_GPIO->INDR & SWDI_PIN ? 1:0;
}

__STATIC_FORCEINLINE uint32_t PIN_SWDIO_TMS_IN2(void)
{
    // return GPIO_ReadInputDataBit(SWD_GPIO, SWDI_PIN);
    return SWD_GPIO->INDR & SWDI_PIN ? 0x80000000:0;
}

/**
 * @brief 设置 SWDIO/TMS 引脚输出电平为高
 *
 * @return None
 */
__STATIC_FORCEINLINE void PIN_SWDIO_TMS_SET(void)
{
    GPIO_SetBits(SWD_GPIO, SWDO_PIN);
}

/**
 * @brief 设置 SWDIO/TMS 引脚输出电平为低
 *
 * @return None
 */
__STATIC_FORCEINLINE void PIN_SWDIO_TMS_CLR(void)
{
    GPIO_ResetBits(SWD_GPIO, SWDO_PIN);
}

#include "drv_spi.h"

__STATIC_FORCEINLINE void PIN_SWDIR_OUTPUT(void)
{
    GPIO_SetBits(SWD_GPIO, SWDIR_PIN);
}

__STATIC_FORCEINLINE void PIN_SWDIR_INPUT(void)
{
    GPIO_ResetBits(SWD_GPIO, SWDIR_PIN);
}

/**
 * @brief 配置 SWDIO 引脚为输出模式
 *
 * @return None
 */
__STATIC_FORCEINLINE void PIN_SWDIO_OUT_ENABLE(void)
{
    drv_spi_gpio_mux_gpio_out();
    // GPIO_SetBits (SWD_GPIO, SWDIR_PIN);
}

/**
 * @brief 配置 SWDIO 引脚为输入模式
 *
 * @return None
 */
__STATIC_FORCEINLINE void PIN_SWDIO_OUT_DISABLE(void)
{
    drv_spi_gpio_mux_gpio_in();
    // GPIO_ResetBits (SWD_GPIO, SWDIR_PIN);
}

/**
 * @brief 读取 SWDIO 引脚当前输入电平
 *
 * @return 电平状态
 */
__STATIC_FORCEINLINE uint32_t PIN_SWDIO_IN(void)
{
    return PIN_SWDIO_TMS_IN();
}

/**
 * @brief 设置 SWDIO 引脚输出电平
 *
 * @param bit 最低位为输出电平
 * @return None
 */
__STATIC_FORCEINLINE void PIN_SWDIO_OUT(uint32_t bit)
{
    if (bit & 0x01)
    {
        PIN_SWDIO_TMS_SET();
    }
    else
    {
        PIN_SWDIO_TMS_CLR();
    }
}

// TDI 引脚 ---------------------------------------------

/**
 * @brief 读取 TDI 引脚输出电平
 *
 * @return 电平状态
 */
__STATIC_FORCEINLINE uint32_t PIN_TDI_IN(void)
{
    return GPIO_ReadOutputDataBit(JTAG_GPIO, JTDI_PIN);
}

/**
 * @brief 设置 TDI 引脚输出电平
 *
 * @param bit 输出电平状态
 * @return None
 */
__STATIC_FORCEINLINE void PIN_TDI_OUT(uint32_t bit)
{
    if (bit & 0x01)
    {
        GPIO_SetBits(JTAG_GPIO, JTDI_PIN);
    }
    else
    {
        GPIO_ResetBits(JTAG_GPIO, JTDI_PIN);
    }
}

// TDO 引脚 ---------------------------------------------

/**
 * @brief 读取 TDO 引脚输入电平
 *
 * @return 电平状态
 */
__STATIC_FORCEINLINE uint32_t PIN_TDO_IN(void)
{
    return GPIO_ReadInputDataBit(JTAG_GPIO, JTDO_PIN);
}

// nTRST 引脚 -------------------------------------------

/**
 * @brief 读取 nTRST 引脚输出电平
 *
 * @return 电平状态
 */
__STATIC_FORCEINLINE uint32_t PIN_nTRST_IN(void)
{
    return GPIO_ReadOutputDataBit(JTRST_GPIO, JTRST_PIN);
}

/**
 * @brief 设置 nTRST 引脚输出电平
 *
 * @param bit 输出电平，0为复位状态，1为释放状态
 * @return None
 */
__STATIC_FORCEINLINE void PIN_nTRST_OUT(uint32_t bit)
{
    if (bit & 0x01)
    {
        GPIO_SetBits(JTRST_GPIO, JTRST_PIN);
    }
    else
    {
        GPIO_ResetBits(JTRST_GPIO, JTRST_PIN);
    }
}

// nRESET 引脚 ------------------------------------------

/**
 * @brief 读取 nRESET 引脚输出电平
 *
 * @return 电平状态
 */
__STATIC_FORCEINLINE uint32_t PIN_nRESET_IN(void)
{
    return (!GPIO_ReadOutputDataBit(SRST_GPIO, SRST_PIN));
}

/**
 * @brief 设置 nRESET 引脚输出电平
 *
 * @param bit 输出电平，0为复位状态，1为释放状态
 * @return None
 */
__STATIC_FORCEINLINE void PIN_nRESET_OUT(uint32_t bit)
{
    if (bit & 0x01)
    {
        GPIO_ResetBits(SRST_GPIO, SRST_PIN);
    }
    else
    {
        GPIO_SetBits(SRST_GPIO, SRST_PIN);
    }
}

// LED 引脚 ------------------------------------------

/**
 * @brief 连接指示 LED 状态设置
 *
 * @param bit LED 是否点亮
 * @return None
 */
__STATIC_INLINE void LED_CONNECTED_OUT(uint32_t bit)
{
}

/**
 * @brief 运行指示 LED 状态设置
 *
 * @param bit LED 是否点亮
 * @return None
 */
__STATIC_INLINE void LED_RUNNING_OUT(uint32_t bit)
{
    if (bit & 0x01)
    {
        GPIO_SetBits(GPIOC, GPIO_Pin_7);
    }
    else
    {
        GPIO_ResetBits(GPIOC, GPIO_Pin_7);
    }
}

/**
 * @brief 通过特定方式复位目标板
 *
 * @return 0为未执行特定复位方式，1为执行完成特定复位动作
 */
__STATIC_INLINE uint8_t RESET_TARGET(void)
{
    return (0U);
}

#endif
