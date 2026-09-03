/*
 * Boot Port Board Implementation for HPM5300
 *
 * Board-level initialization for bootloader on akaLink
 */

#include "boot_port_board.h"
#include "board.h"
#include "usb_config.h"
#include "hpm_clock_drv.h"
#include "hpm_gpio_drv.h"
#include "hpm_usb_drv.h"
#include "hpm_interrupt.h"
#include "pinmux.h"

void boot_port_board_init(void)
{
    /* Initialize system clock */
    board_init_clock();

    /* Initialize console for debug output */
    board_init_console();

    /* Initialize boot pin GPIO */
    boot_port_board_init_bootpin();

    init_py_pins_as_pgpio();
    init_power_pins();
    init_gpio_swj_pins();

    /* Init LEDs */
    init_led_pins();

    /* Initialize USB controller */
    board_init_usb((USB_Type *)CONFIG_HPM_USBD_BASE);
    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 2);
}

void boot_port_board_deinit(void)
{
    board_led1_off();
    board_led2_off();
}

void boot_port_board_init_bootpin(void)
{
    init_dfu_pins();
}

bool boot_port_board_read_bootpin(void)
{
#ifdef BOARD_APP_GPIO_CTRL
    uint8_t pin_state = gpio_read_pin(BOARD_APP_GPIO_CTRL,
                                      BOARD_APP_GPIO_INDEX,
                                      BOARD_APP_GPIO_PIN);
    return (pin_state == BOARD_BTN_PRESSED_VALUE);
#else
    return false;
#endif
}
