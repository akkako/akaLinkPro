/*
 * Copyright (c) 2023 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/*
 * Note:
 *  PY and PZ IOs: if any SOC pin function needs to be routed to these IOs,
 *  besides of IOC, PIOC/BIOC needs to be configured SOC_GPIO_X_xx, so that
 *  expected SoC function can be enabled on these IOs.
 *
 */
#include "board.h"
#include "pinmux.h"
#include "hpm_gpio_drv.h"
#include "hpm_gpiom_drv.h"

// Miscellaneous
#define DFU_PIN IOC_PAD_PA10
#define LED1_PIN IOC_PAD_PB11
#define LED2_PIN IOC_PAD_PB12
#define SEL0_PIN IOC_PAD_PB08
#define SEL1_PIN IOC_PAD_PB09

// power
#define BUS5V_EN_PIN IOC_PAD_PB13
#define ADC_VREF_PIN IOC_PAD_PB10

// UART0
#define ISP_TX_PIN IOC_PAD_PA00
#define ISP_RX_PIN IOC_PAD_PA01

// UART3
#define AUX_RX_PIN IOC_PAD_PB14
#define AUX_TX_PIN IOC_PAD_PB15

// UART2/SWJ
#define JTDI_TXD_PIN IOC_PAD_PA08
#define JTDO_RXD_PIN IOC_PAD_PA09

// SWJ
#define JTCK_PIN IOC_PAD_PA27
#define JTMS_IN_PIN IOC_PAD_PA28
#define JTMS_OUT_PIN IOC_PAD_PA29
#define JTMS_DIR_PIN IOC_PAD_PA30
#define JTRST_PIN IOC_PAD_PA31
#define nRESET_PIN IOC_PAD_PA26

// USB
#define USB_DP_PIN IOC_PAD_PA24
#define USB_DN_PIN IOC_PAD_PA25

void init_py_pins_as_pgpio(void)
{
    /* Set PY00-PY05 default function to PGPIO */
    HPM_PIOC->PAD[IOC_PAD_PY00].FUNC_CTL = PIOC_PY00_FUNC_CTL_PGPIO_Y_00;
    HPM_PIOC->PAD[IOC_PAD_PY01].FUNC_CTL = PIOC_PY01_FUNC_CTL_PGPIO_Y_01;
    HPM_PIOC->PAD[IOC_PAD_PY02].FUNC_CTL = PIOC_PY02_FUNC_CTL_PGPIO_Y_02;
    HPM_PIOC->PAD[IOC_PAD_PY03].FUNC_CTL = PIOC_PY03_FUNC_CTL_PGPIO_Y_03;
    HPM_PIOC->PAD[IOC_PAD_PY04].FUNC_CTL = PIOC_PY04_FUNC_CTL_PGPIO_Y_04;
    HPM_PIOC->PAD[IOC_PAD_PY05].FUNC_CTL = PIOC_PY05_FUNC_CTL_PGPIO_Y_05;
}

void init_gpio_swj_pins(void)
{
    // set alternate function as gpio
    HPM_IOC->PAD[JTDI_TXD_PIN].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[JTDI_RXD_PIN].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[JTCK_PIN].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[JTMS_IN_PIN].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[JTMS_OUT_PIN].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[JTMS_DIR_PIN].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[JTRST_PIN].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[nRESET_PIN].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);

    // set pad parameter
    HPM_IOC->PAD[JTDI_TXD_PIN].PAD_CTL = IOC_PAD_PAD_CTL_SR_SET(1) |
                                         IOC_PAD_PAD_CTL_SPD_SET(3);

    // set pin controller
    gpiom_set_pin_controller(HPM_GPIOM, GPIO_GET_PORT_INDEX(JTDI_TXD_PIN), GPIO_GET_PIN_INDEX(JTDI_TXD_PIN), gpiom_soc_gpio0);

    // set output/input mode
    gpio_set_pin_output(HPM_FGPIO, GPIO_GET_PORT_INDEX(JTDI_TXD_PIN), GPIO_GET_PIN_INDEX(JTDI_TXD_PIN));

    // set pin default state
    gpio_write_pin(HPM_FGPIO, GPIO_GET_PORT_INDEX(JTDI_TXD_PIN), GPIO_GET_PIN_INDEX(JTDI_TXD_PIN), 1);
}

/**
 * @brief Init UART pins
 * @param ptr UART_Type instance
 */
void init_uart_pins(UART_Type *ptr)
{
    if (ptr == HPM_UART0)
    {
        HPM_IOC->PAD[ISP_TX_PIN].FUNC_CTL = IOC_PA00_FUNC_CTL_UART0_TXD;
        HPM_IOC->PAD[ISP_RX_PIN].FUNC_CTL = IOC_PA01_FUNC_CTL_UART0_RXD;
    }
    else if (ptr == HPM_UART2)
    {
        HPM_IOC->PAD[JTDI_TXD_PIN].FUNC_CTL = IOC_PA08_FUNC_CTL_UART2_TXD;
        HPM_IOC->PAD[JTDI_RXD_PIN].FUNC_CTL = IOC_PA09_FUNC_CTL_UART2_RXD;
    }
    else if (ptr == HPM_UART3)
    {
        HPM_IOC->PAD[AUX_TX_PIN].FUNC_CTL = IOC_PB15_FUNC_CTL_UART3_TXD;
        HPM_IOC->PAD[AUX_RX_PIN].FUNC_CTL = IOC_PB14_FUNC_CTL_UART3_RXD;
    }
    else
    {
        ;
    }
}

/**
 * @brief Init LED pins
 * @param None
 */
void init_led_pins(void)
{
    uint32_t pad_ctl_slow = IOC_PAD_PAD_CTL_HYS_SET(0) |
                            IOC_PAD_PAD_CTL_PRS_SET(0) |
                            IOC_PAD_PAD_CTL_PS_SET(0) |
                            IOC_PAD_PAD_CTL_PE_SET(0) |
                            IOC_PAD_PAD_CTL_KE_SET(0) |
                            IOC_PAD_PAD_CTL_OD_SET(0) |
                            IOC_PAD_PAD_CTL_SR_SET(0) |
                            IOC_PAD_PAD_CTL_SPD_SET(0) |
                            IOC_PAD_PAD_CTL_DS_SET(0);

    // set alternate function as gpio
    HPM_IOC->PAD[LED1_PIN].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[LED2_PIN].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);

    // set pad parameter
    HPM_IOC->PAD[LED1_PIN].PAD_CTL = pad_ctl_slow;
    HPM_IOC->PAD[LED2_PIN].PAD_CTL = pad_ctl_slow;

    // set pin controller
    gpiom_set_pin_controller(HPM_GPIOM, GPIO_GET_PORT_INDEX(LED1_PIN), GPIO_GET_PIN_INDEX(LED1_PIN), gpiom_soc_gpio0);
    gpiom_set_pin_controller(HPM_GPIOM, GPIO_GET_PORT_INDEX(LED2_PIN), GPIO_GET_PIN_INDEX(LED2_PIN), gpiom_soc_gpio0);

    // set output/input mode
    gpio_set_pin_output(HPM_GPIO0, GPIO_GET_PORT_INDEX(LED1_PIN), GPIO_GET_PIN_INDEX(LED1_PIN));
    gpio_set_pin_output(HPM_GPIO0, GPIO_GET_PORT_INDEX(LED2_PIN), GPIO_GET_PIN_INDEX(LED2_PIN));

    // set pin default state
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(LED1_PIN), GPIO_GET_PIN_INDEX(LED1_PIN), 0);
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(LED2_PIN), GPIO_GET_PIN_INDEX(LED2_PIN), 0);
}

/**
 * @brief Init power control and sense pin
 * @param None
 */
void init_power_pins(void)
{
    // set alternate function as gpio
    HPM_IOC->PAD[BUS5V_EN_PIN].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);

    // set pad parameter
    HPM_IOC->PAD[BUS5V_EN_PIN].PAD_CTL = IOC_PAD_PAD_CTL_HYS_SET(0) |
                                         IOC_PAD_PAD_CTL_PRS_SET(0) |
                                         IOC_PAD_PAD_CTL_PS_SET(0) |
                                         IOC_PAD_PAD_CTL_PE_SET(0) |
                                         IOC_PAD_PAD_CTL_KE_SET(0) |
                                         IOC_PAD_PAD_CTL_OD_SET(0) |
                                         IOC_PAD_PAD_CTL_SR_SET(0) |
                                         IOC_PAD_PAD_CTL_SPD_SET(0) |
                                         IOC_PAD_PAD_CTL_DS_SET(0);

    // set pin controller
    gpiom_set_pin_controller(HPM_GPIOM, GPIO_GET_PORT_INDEX(BUS5V_EN_PIN), GPIO_GET_PIN_INDEX(BUS5V_EN_PIN), gpiom_soc_gpio0);

    // set output/input mode
    gpio_set_pin_output(HPM_GPIO0, GPIO_GET_PORT_INDEX(BUS5V_EN_PIN), GPIO_GET_PIN_INDEX(BUS5V_EN_PIN));

    // set pin default state
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(BUS5V_EN_PIN), GPIO_GET_PIN_INDEX(BUS5V_EN_PIN), 0);

    // set alternate function as analog
    HPM_IOC->PAD[BUS5V_EN_PIN].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
}

void init_model_sel_pins(void)
{
}

void init_dfu_pins(void)
{
    uint32_t pad_ctl = IOC_PAD_PAD_CTL_HYS_SET(1) | // hyster enable
                       IOC_PAD_PAD_CTL_PRS_SET(0) | // pull res 100k
                       IOC_PAD_PAD_CTL_PS_SET(0) |  // pull down
                       IOC_PAD_PAD_CTL_PE_SET(1) |  // pull enable
                       IOC_PAD_PAD_CTL_KE_SET(0) |
                       IOC_PAD_PAD_CTL_OD_SET(0) |
                       IOC_PAD_PAD_CTL_SR_SET(0) |
                       IOC_PAD_PAD_CTL_SPD_SET(0) |
                       IOC_PAD_PAD_CTL_DS_SET(0);

    HPM_IOC->PAD[DFU_PIN].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[DFU_PIN].PAD_CTL = pad_ctl;

    gpiom_set_pin_controller(HPM_GPIOM, GPIO_GET_PORT_INDEX(DFU_PIN), GPIO_GET_PIN_INDEX(DFU_PIN), gpiom_soc_gpio0);
    gpio_set_pin_input(HPM_GPIO0, GPIO_GET_PORT_INDEX(DFU_PIN), GPIO_GET_PIN_INDEX(DFU_PIN));
}

void init_usb_pins(USB_Type *ptr)
{
    if (ptr == HPM_USB0)
    {
        HPM_IOC->PAD[USB_DP_PIN].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
        HPM_IOC->PAD[USB_DN_PIN].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;
    }
}
