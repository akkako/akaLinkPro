#include "board.h"
#include "clock.h"
#include "hpm_gpio_drv.h"
#include "hpm_gptmr_drv.h"
#include "hpm_adc16_drv.h"
#include "hpm_interrupt.h"
#include "led_state.h"
#include "api_param.h"

/* LED pins (active high): LED1 blue = PB11, LED2 yellow = PB12. */
#define LED1_PIN IOC_PAD_PB11
#define LED2_PIN IOC_PAD_PB12

/* External reference is divided by two 10k resistors on PB10 (ADC0.2). */
#define LED_ADC_PIN IOC_PAD_PB10
#define LED_ADC_BASE HPM_ADC0
#define LED_ADC_CH (2U)
#define LED_ADC_FULL_SCALE_MV (3300U)
#define LED_ADC_SAMPLE_CYCLE (20U)

/* Dedicated timer tick; GPTMR0 is already used by the CDC flush timer. */
#define LED_TICK_TIMER HPM_GPTMR1
#define LED_TICK_TIMER_IRQ IRQn_GPTMR1
#define LED_TICK_TIMER_CH (0U)
#define LED_TICK_PERIOD_MS (50U)

/* 5Hz blink = 200ms period = 100ms on + 100ms off = 2 ticks per half. */
#define LED_BLINK_HALF_TICKS (2U)

#define LED_VREF_ON_PERCENT (90U)
#define LED_VREF_OFF_PERCENT (85U)

static volatile uint8_t s_dap_running;
static volatile uint8_t s_dap_connected;

/* Incremented by the USB/UART paths; the tick converts a change into a hold. */
static volatile uint32_t s_tx_activity;
static volatile uint32_t s_rx_activity;
static uint32_t s_tx_activity_seen;
static uint32_t s_rx_activity_seen;
static uint8_t s_tx_hold;
static uint8_t s_rx_hold;

static uint32_t s_tick;
static uint8_t s_vref_on;
static uint8_t s_adc_ready;
static volatile uint16_t s_external_mv;

static void led_write(uint16_t pin, uint8_t on)
{
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(pin), GPIO_GET_PIN_INDEX(pin), on ? 1U : 0U);
}

void led_state_set_dap_running(uint8_t on)
{
    s_dap_running = on ? 1U : 0U;
}

void led_state_set_dap_connected(uint8_t on)
{
    s_dap_connected = on ? 1U : 0U;
}

void led_state_notify_uart_tx(uint32_t bytes)
{
    if (bytes)
    {
        s_tx_activity++;
    }
}

void led_state_notify_uart_rx(uint32_t bytes)
{
    if (bytes)
    {
        s_rx_activity++;
    }
}

uint16_t led_state_get_external_mv(void)
{
    return s_external_mv;
}

static void led_adc_init(void)
{
    adc16_config_t cfg;
    adc16_channel_config_t ch_cfg;

    init_adc0_bus_clock();

    adc16_get_default_config(&cfg);
    cfg.res = adc16_res_16_bits;
    cfg.conv_mode = adc16_conv_mode_oneshot;
    cfg.adc_clk_div = adc16_clock_divider_4;
#if !defined(HPM_IP_FEATURE_ADC16_FORCE_SYNC_AHB) || !HPM_IP_FEATURE_ADC16_FORCE_SYNC_AHB
    cfg.sel_sync_ahb = true;
#endif
    if (adc16_init(LED_ADC_BASE, &cfg) != status_success)
    {
        return;
    }

    adc16_get_channel_default_config(&ch_cfg);
    ch_cfg.ch = LED_ADC_CH;
    ch_cfg.sample_cycle = LED_ADC_SAMPLE_CYCLE;
    if (adc16_init_channel(LED_ADC_BASE, &ch_cfg) != status_success)
    {
        return;
    }

#if defined(ADC_SOC_BUSMODE_ENABLE_CTRL_SUPPORT) && ADC_SOC_BUSMODE_ENABLE_CTRL_SUPPORT
    adc16_enable_oneshot_mode(LED_ADC_BASE);
#endif

    s_adc_ready = 1U;
}

/* Sample the divided reference and update the mode-5 state with hysteresis:
 * on at 90% of the setting, off at 85%. */
static void led_vref_update(void)
{
    uint16_t raw;
    uint32_t ext_mv;
    uint32_t set_mv = g_param.vref_mv;
    uint32_t on_mv;
    uint32_t off_mv;

    if (!s_adc_ready)
    {
        return;
    }
    if (adc16_get_oneshot_result(LED_ADC_BASE, LED_ADC_CH, &raw) != status_success)
    {
        return;
    }

    /* result / 65535 * 3.3V, then x2 to undo the 10k/10k divider. */
    ext_mv = (uint32_t)raw * (LED_ADC_FULL_SCALE_MV * 2U) / 65535U;
    s_external_mv = (uint16_t)ext_mv;

    if (set_mv < 1800U)
    {
        set_mv = 1800U;
    }
    else if (set_mv > 5000U)
    {
        set_mv = 5000U;
    }
    on_mv = set_mv * LED_VREF_ON_PERCENT / 100U;
    off_mv = set_mv * LED_VREF_OFF_PERCENT / 100U;

    if (s_vref_on)
    {
        if (ext_mv < off_mv)
        {
            s_vref_on = 0U;
        }
    }
    else
    {
        if (ext_mv >= on_mv)
        {
            s_vref_on = 1U;
        }
    }
}

static uint8_t led_mode_eval(uint8_t mode, uint8_t blink_on, uint8_t tx_on, uint8_t rx_on)
{
    switch (mode)
    {
    case LED_MODE_DAP_RUNNING:
        return s_dap_running;
    case LED_MODE_DAP_CONNECT:
        return s_dap_connected;
    case LED_MODE_DAP_ANY:
        return (s_dap_running || s_dap_connected) ? blink_on : 1U;
    case LED_MODE_POWER:
        return 1U;
    case LED_MODE_VREF:
        return s_vref_on;
    case LED_MODE_CDC_TX:
        return tx_on;
    case LED_MODE_CDC_RX:
        return rx_on;
    case LED_MODE_CDC_TX_RX:
        return (tx_on || rx_on) ? 1U : 0U;
    case LED_MODE_OFF:
    default:
        return 0U;
    }
}

static void led_state_tick(void)
{
    uint8_t blink_on;
    uint8_t tx_on;
    uint8_t rx_on;

    s_tick++;

    /* Minimum on-time of one tick (50ms) per activity burst. A change in the
     * activity counter re-arms the hold, so continuous traffic keeps it lit. */
    if (s_tx_activity != s_tx_activity_seen)
    {
        s_tx_activity_seen = s_tx_activity;
        s_tx_hold = 1U;
    }
    else if (s_tx_hold)
    {
        s_tx_hold--;
    }

    if (s_rx_activity != s_rx_activity_seen)
    {
        s_rx_activity_seen = s_rx_activity;
        s_rx_hold = 1U;
    }
    else if (s_rx_hold)
    {
        s_rx_hold--;
    }

    led_vref_update();

    /* 5Hz square wave: LED_BLINK_HALF_TICKS ticks on, same amount off. */
    blink_on = (((s_tick - 1U) % (LED_BLINK_HALF_TICKS * 2U)) < LED_BLINK_HALF_TICKS) ? 1U : 0U;
    tx_on = (s_tx_hold != 0U) ? 1U : 0U;
    rx_on = (s_rx_hold != 0U) ? 1U : 0U;

    led_write(LED1_PIN, led_mode_eval(g_param.led1_mode, blink_on, tx_on, rx_on));
    led_write(LED2_PIN, led_mode_eval(g_param.led2_mode, blink_on, tx_on, rx_on));
}

SDK_DECLARE_EXT_ISR_M(LED_TICK_TIMER_IRQ, led_tick_isr)
void led_tick_isr(void)
{
    if (!gptmr_check_status(LED_TICK_TIMER, GPTMR_CH_RLD_STAT_MASK(LED_TICK_TIMER_CH)))
    {
        return;
    }
    gptmr_clear_status(LED_TICK_TIMER, GPTMR_CH_RLD_STAT_MASK(LED_TICK_TIMER_CH));

    led_state_tick();
}

static void led_tick_timer_init(void)
{
    gptmr_channel_config_t cfg;
    uint32_t ticks;

    init_gptmr1_clock();

    ticks = (uint32_t)((uint64_t)clock_get_frequency(clock_gptmr1) * LED_TICK_PERIOD_MS / 1000ULL);
    if (ticks == 0U)
    {
        ticks = 1U;
    }

    gptmr_channel_get_default_config(LED_TICK_TIMER, &cfg);
    cfg.mode = gptmr_work_mode_no_capture;
    cfg.reload = ticks;
    cfg.cmp[0] = 0U;
    cfg.cmp[1] = 0U;
    (void)gptmr_channel_config(LED_TICK_TIMER, LED_TICK_TIMER_CH, &cfg, false);
    gptmr_enable_irq(LED_TICK_TIMER, GPTMR_CH_RLD_IRQ_MASK(LED_TICK_TIMER_CH));
    gptmr_channel_reset_count(LED_TICK_TIMER, LED_TICK_TIMER_CH);
    gptmr_start_counter(LED_TICK_TIMER, LED_TICK_TIMER_CH);
    intc_m_enable_irq_with_priority(LED_TICK_TIMER_IRQ, 3);
}

void led_state_init(void)
{
    led_write(LED1_PIN, 0U);
    led_write(LED2_PIN, 0U);

    led_adc_init();
    led_tick_timer_init();
}
