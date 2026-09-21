#ifndef __LED_STATE_H__
#define __LED_STATE_H__

#include <stdint.h>

/* LED display modes, selectable per LED (LED1 = PB11, LED2 = PB12).
 * The numeric values match the mode numbers in led_state.txt. */
#define LED_MODE_DAP_RUNNING (1U) /* DAP RUNNING: 1 = on, 0 = off */
#define LED_MODE_DAP_CONNECT (2U) /* DAP CONNECT: 1 = on, 0 = off */
#define LED_MODE_DAP_ANY (3U)     /* RUNNING || CONNECT: blink 5Hz, else on */
#define LED_MODE_POWER (4U)       /* Debugger powered: always on */
#define LED_MODE_VREF (5U)        /* External reference above threshold */
#define LED_MODE_CDC_TX (6U)      /* Debugger UART TX activity, min 50ms */
#define LED_MODE_CDC_RX (7U)      /* Debugger UART RX activity, min 50ms */
#define LED_MODE_CDC_TX_RX (8U)   /* Debugger UART TX or RX activity */
#define LED_MODE_OFF (9U)         /* Always off */

#ifdef __cplusplus
extern "C"
{
#endif

    /* Configure LED GPIOs, ADC (PB10) and the periodic tick timer. */
    void led_state_init(void);

    /* Called from the CMSIS-DAP HostStatus command. */
    void led_state_set_dap_running(uint8_t on);
    void led_state_set_dap_connected(uint8_t on);

    /* Called when the debugger's UART actually transmits / receives bytes. */
    void led_state_notify_uart_tx(uint32_t bytes);
    void led_state_notify_uart_rx(uint32_t bytes);

    /* Last measured external reference voltage in mV (already x2, undoing the
     * 10k/10k divider). Returns 0 until the first ADC sample is available. */
    uint16_t led_state_get_external_mv(void);

#ifdef __cplusplus
}
#endif

#endif /* __LED_STATE_H__ */
