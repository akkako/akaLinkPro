/*
 * Copyright (c) 2026 HPMicro
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/*
 * Note:
 * PY and PZ IOs: if any SOC pin function needs to be routed to these IOs,
 * besides of IOC, PIOC/BIOC needs to be configured SOC_GPIO_X_xx, so that
 * expected SoC function can be enabled on these IOs.
 */

#include "pinmux.h"
#include "board.h"
#include "hpm_trgm_drv.h"
#include "hpm_gpio_drv.h"
#include "hpm_gpiom_drv.h"


static void gpiom_config_pin_to_gpio0(uint16_t gpio_index)
{
    gpiom_set_pin_controller(HPM_GPIOM,
                             GPIO_GET_PORT_INDEX(gpio_index),
                             GPIO_GET_PIN_INDEX(gpio_index),
                             gpiom_soc_gpio0);
    gpiom_enable_pin_visibility(HPM_GPIOM,
                                GPIO_GET_PORT_INDEX(gpio_index),
                                GPIO_GET_PIN_INDEX(gpio_index),
                                gpiom_soc_gpio0);
}

static void init_unused_pin_as_input_pull_down(uint16_t gpio_index)
{
    HPM_IOC->PAD[gpio_index].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[gpio_index].PAD_CTL = IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(0);

    gpiom_config_pin_to_gpio0(gpio_index);
    gpio_set_pin_input(HPM_GPIO0, GPIO_GET_PORT_INDEX(gpio_index), GPIO_GET_PIN_INDEX(gpio_index));
    
}

static void init_unused_pin_as_input_no_pull(uint16_t gpio_index)
{
    HPM_IOC->PAD[gpio_index].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[gpio_index].PAD_CTL = 0;

    gpiom_config_pin_to_gpio0(gpio_index);
    gpio_set_pin_input(HPM_GPIO0, GPIO_GET_PORT_INDEX(gpio_index), GPIO_GET_PIN_INDEX(gpio_index));
    
}

/**
 * @brief Set PY00-PY05 default function to PGPIO
 * @param None
 */
void init_py_pins_as_pgpio(void)
{
    HPM_PIOC->PAD[IOC_PAD_PY00].FUNC_CTL = PIOC_PY00_FUNC_CTL_PGPIO_Y_00;
    HPM_PIOC->PAD[IOC_PAD_PY01].FUNC_CTL = PIOC_PY01_FUNC_CTL_PGPIO_Y_01;
}

/**
 * @brief Init UART0 Console print pin
 * @param None
 */
void init_uart0_pins(void)
{
    HPM_IOC->PAD[IOC_PAD_PA00].FUNC_CTL = IOC_PA00_FUNC_CTL_UART0_TXD;
    HPM_IOC->PAD[IOC_PAD_PA01].FUNC_CTL = IOC_PA01_FUNC_CTL_UART0_RXD;
}

/*
 * configure pad setting: pull enable and pull down, schmitt trigger enable
 * enable schmitt trigger to eliminate jitter of pin used as button
 */
void init_button_pins(void)
{
    /* Button */
    HPM_IOC->PAD[IOC_PAD_PA03].FUNC_CTL = IOC_PA03_FUNC_CTL_GPIO_A_03;
    HPM_IOC->PAD[IOC_PAD_PA03].PAD_CTL = IOC_PAD_PAD_CTL_HYS_SET(1) | IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(0);
}

/**
 * @brief Init USB0 pins
 * @param None
 */
void init_usb0_pins(void)
{
    HPM_IOC->PAD[IOC_PAD_PA24].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;

    HPM_IOC->PAD[IOC_PAD_PA25].FUNC_CTL = IOC_PAD_FUNC_CTL_ANALOG_MASK;

    /* USB0_ID */
    HPM_IOC->PAD[IOC_PAD_PY00].FUNC_CTL = IOC_PY00_FUNC_CTL_USB0_ID;
    HPM_PIOC->PAD[IOC_PAD_PY00].FUNC_CTL = PIOC_PY00_FUNC_CTL_SOC_GPIO_Y_00;

    /* USB0_OC */
    HPM_IOC->PAD[IOC_PAD_PY01].FUNC_CTL = IOC_PY01_FUNC_CTL_USB0_OC;
    HPM_PIOC->PAD[IOC_PAD_PY01].FUNC_CTL = PIOC_PY01_FUNC_CTL_SOC_GPIO_Y_01;
}

/**
 * @brief Init UART3 as VCOM port
 * @param None
 */
void init_uart3_pins_as_uart(void)
{
    HPM_IOC->PAD[IOC_PAD_PB15].FUNC_CTL = IOC_PB15_FUNC_CTL_UART3_TXD;
    HPM_IOC->PAD[IOC_PAD_PB14].FUNC_CTL = IOC_PB14_FUNC_CTL_UART3_RXD;
}

/*
 * for uart_lin case, need to configure pin as gpio to sent break signal
 * pull-up
 */
void init_uart3_pin_as_gpio_low(void)
{
    HPM_IOC->PAD[BOARD_PIN_UART_TXD].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[BOARD_PIN_UART_TXD].PAD_CTL = IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(0);

    HPM_IOC->PAD[BOARD_PIN_UART_RXD].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[BOARD_PIN_UART_RXD].PAD_CTL = IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(0);

    HPM_IOC->PAD[BOARD_PIN_UART_RTS].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[BOARD_PIN_UART_RTS].PAD_CTL = IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(0);

    HPM_IOC->PAD[BOARD_PIN_UART_DTR].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[BOARD_PIN_UART_DTR].PAD_CTL = IOC_PAD_PAD_CTL_PE_SET(1) | IOC_PAD_PAD_CTL_PS_SET(0);

    gpiom_config_pin_to_gpio0(BOARD_PIN_UART_TXD);
    gpiom_config_pin_to_gpio0(BOARD_PIN_UART_RXD);
    gpiom_config_pin_to_gpio0(BOARD_PIN_UART_RTS);
    gpiom_config_pin_to_gpio0(BOARD_PIN_UART_DTR);

    gpio_set_pin_output(HPM_GPIO0, GPIO_GET_PORT_INDEX(BOARD_PIN_UART_TXD), GPIO_GET_PIN_INDEX(BOARD_PIN_UART_TXD));
    gpio_set_pin_output(HPM_GPIO0, GPIO_GET_PORT_INDEX(BOARD_PIN_UART_RTS), GPIO_GET_PIN_INDEX(BOARD_PIN_UART_RTS));
    gpio_set_pin_output(HPM_GPIO0, GPIO_GET_PORT_INDEX(BOARD_PIN_UART_DTR), GPIO_GET_PIN_INDEX(BOARD_PIN_UART_DTR));
    gpio_set_pin_input(HPM_GPIO0, GPIO_GET_PORT_INDEX(BOARD_PIN_UART_RXD), GPIO_GET_PIN_INDEX(BOARD_PIN_UART_RXD));

    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(BOARD_PIN_UART_TXD), GPIO_GET_PIN_INDEX(BOARD_PIN_UART_TXD), 0);
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(BOARD_PIN_UART_RTS), GPIO_GET_PIN_INDEX(BOARD_PIN_UART_RTS), 0);
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(BOARD_PIN_UART_DTR), GPIO_GET_PIN_INDEX(BOARD_PIN_UART_DTR), 0);
}

/*
 * @brief Init unused pins as input
 * @param None
 */
void init_unused_pin_as_input(void)
{
    init_unused_pin_as_input_pull_down(BOARD_PIN_UNUSED_PORTEN);
    init_unused_pin_as_input_pull_down(BOARD_PIN_UNUSED_PWM);
    init_unused_pin_as_input_pull_down(BOARD_PIN_UNUSED_CS1);
    init_unused_pin_as_input_pull_down(BOARD_PIN_UNUSED_CS2);
    init_unused_pin_as_input_pull_down(BOARD_PIN_UNUSED_PEN);
    init_unused_pin_as_input_pull_down(BOARD_PIN_UNUSED_VREF);
    init_unused_pin_as_input_pull_down(BOARD_PIN_UNUSED_TVCC);
    init_unused_pin_as_input_no_pull(BOARD_PIN_UNUSED_JTCK);
}

/**
 * @brief Init JTAG SWD pins as default state
 * @param None
 */
void init_jtag_swd_pin(void)
{
}