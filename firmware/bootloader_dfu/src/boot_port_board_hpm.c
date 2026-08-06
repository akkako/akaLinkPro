/*
 * Boot Port Board Implementation for HPM6E80
 *
 * Board-level initialization for bootloader on HPM6E00EVK
 */

#include "boot_port_board.h"
#include "board.h"
#include "usb_config.h"
#include "hpm_clock_drv.h"
#include "hpm_gpio_drv.h"
#include "hpm_usb_drv.h"
#include "hpm_interrupt.h"
#include "boot_log.h"

void boot_port_board_init(void)
{
    /* Initialize system clock */
    board_init_clock();

    /* Initialize console for debug output */
    board_init_console();

    /* Initialize boot pin GPIO */
    boot_port_board_init_bootpin();

    /* Initialize LED for visual feedback (optional, not all boards have it) */
#ifdef BOARD_LED_GPIO_CTRL
    /* board_init_led_pins may not be available on all boards */
    extern void board_init_led_pins(void) __attribute__((weak));
    if (board_init_led_pins) {
        board_init_led_pins();
        board_led_write(1);
    }
#endif

    /* Initialize USB controller */
    board_init_usb((USB_Type *)CONFIG_HPM_USBD_BASE);
    intc_set_irq_priority(CONFIG_HPM_USBD_IRQn, 2);
}

void boot_port_board_deinit(void)
{
#ifdef BOARD_LED_GPIO_CTRL
    board_led_write(0);
#endif
}

void boot_port_board_init_bootpin(void)
{
#ifdef BOARD_APP_GPIO_CTRL
    board_init_gpio_pins();
#endif
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

void boot_port_board_led_toggle(void)
{
#ifdef BOARD_LED_GPIO_CTRL
    board_led_toggle();
#endif
}
