#include <stdio.h>
#include <hpm_gpiom_soc_drv.h>
#include <hpm_gpio_drv.h>
#include <hpm_gpiom_drv.h>
#include "board.h"
#include "clock.h"
#include "hpm_uart_drv.h"
#include "hpm_debug_console.h"
#include "hpm_dma_mgr.h"
#include "hpm_gptmr_drv.h"
#include "hpm_sysctl_drv.h"
#include "usb_composite.h"
#include "cdc_interface.h"

#define UART_BASE HPM_UART2
#define UART_IRQ IRQn_UART2
#define UART_CLK_NAME clock_uart2
#define UART_RX_DMA HPM_DMA_SRC_UART2_RX
#define UART_RX_DMA_RESOURCE_INDEX (0U)
/* Single RX buffer, DMAV2 infinite-loop. The periodic flush timer drains it
 * into g_uartrx, so it only needs to cover the (short) IRQ latency caused by
 * the SWD delay-sampling critical sections, not the whole SWD block command. */
#define UART_RX_DMA_BUFFER_SIZE (8192U)

/* Periodic timer that moves received bytes from the circular RX DMA buffer
 * into g_uartrx. Using a timer (instead of relying only on the UART IDLE IRQ
 * and the buffer-full TC IRQ) keeps the data flowing even during a continuous
 * SWD transfer, whose critical sections jitter the other interrupts. */
#define UART_FLUSH_TIMER HPM_GPTMR0
#define UART_FLUSH_TIMER_IRQ IRQn_GPTMR0
#define UART_FLUSH_TIMER_CH (0U)
#define UART_FLUSH_INTERVAL_US (500U)

#define UART_TX_DMA HPM_DMA_SRC_UART2_TX
#define UART_TX_DMA_RESOURCE_INDEX (1U)
// #define UART_TX_DMA_BUFFER_SIZE    (8192U)

/* PA06/PA07 are not connected to modem-control nets on this board, so do not
 * drive them by default. Set to 1 if a future board uses DTR/RTS. */
#ifndef UART2_DRIVE_DTR_RTS
#define UART2_DRIVE_DTR_RTS (0)
#endif

/* Software cap for the CDC COM port baud rate. Requests above this value are
 * clamped; requests that cannot be generated exactly are rounded to the
 * closest achievable baud (see uart2_round_baudrate). */
#ifndef UART2_MAX_BAUDRATE
#define UART2_MAX_BAUDRATE (9000000U)
#endif

/* Number of bytes of the single RX buffer that were already copied into
 * g_uartrx. Reset whenever the RX DMA is restarted. */
static volatile uint32_t rb_write_pos = 0;
/* 1 = PA08/PA09 are muxed to UART2 (COM mode), 0 = JTAG owns the pins. */
static volatile uint8_t s_uart2_com_mode = 0;
ATTR_PLACE_AT_NONCACHEABLE_BSS_WITH_ALIGNMENT(4)
uint8_t uart_rx_buf[UART_RX_DMA_BUFFER_SIZE];

static dma_resource_t dma_resource_pools[2];
volatile uint32_t g_uart_tx_transfer_length = 0;
/* Last baud rate actually programmed into UART2 (after clamping/rounding). */
volatile uint32_t g_uart2_applied_baud = 0;

static hpm_stat_t board_uart_dma_config(void);
static void uartx_mux_to_uart(void);
static void uartx_rx_dma_start(void);

static uint32_t PIN_UART_DTR = 0;
static uint32_t PIN_UART_RTS = 0;

/* Read position (in bytes) inside the current lap of the circular RX buffer.
 * The RX channel runs in DMAV2 infinite-loop mode, so the hardware reloads the
 * transfer size on every wrap and dma_get_remaining_transfer_size() always
 * refers to the current lap (no software restart involved). */
static uint32_t uartx_rx_written(void)
{
    dma_resource_t *rx_resource = &dma_resource_pools[UART_RX_DMA_RESOURCE_INDEX];
    if (rx_resource->base == NULL)
    {
        return 0;
    }
    /* Live DMA destination pointer (wraps at the end of the circular buffer).
     * More reliable than the transfer-size counter, which can hold the
     * previous lap's value for a few cycles around a wrap. */
    uint32_t base = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)uart_rx_buf);
    uint32_t cur = rx_resource->base->CHCTRL[rx_resource->channel].DSTADDR;
    if ((cur < base) || (cur > (base + UART_RX_DMA_BUFFER_SIZE)))
    {
        return 0;
    }
    return cur - base;
}

/* Copy the bytes received so far in the circular RX buffer into the ringbuffer.
 * The RX DMA runs in infinite-loop mode (DMAV2), so it wraps around by itself
 * and we never need to disable/restart it while streaming. `rb_write_pos` is
 * the read position inside the current lap; when the hardware write pointer is
 * behind it, a wrap has happened and the tail + head are copied in two parts.
 * Must be called with interrupts disabled (or from an ISR). */
static void uartx_rx_flush_locked(void)
{
    uint32_t written = uartx_rx_written();

    if (written == rb_write_pos)
    {
        return;
    }

    if (written > rb_write_pos)
    {
        chry_ringbuffer_write(&g_uartrx, &uart_rx_buf[rb_write_pos], written - rb_write_pos);
    }
    else
    {
        /* Wrapped: copy the tail then the head. */
        if (rb_write_pos < UART_RX_DMA_BUFFER_SIZE)
        {
            chry_ringbuffer_write(&g_uartrx, &uart_rx_buf[rb_write_pos],
                                  UART_RX_DMA_BUFFER_SIZE - rb_write_pos);
        }
        if (written > 0)
        {
            chry_ringbuffer_write(&g_uartrx, &uart_rx_buf[0], written);
        }
    }
    rb_write_pos = written;
}

/* (Re)start the circular RX DMA from the beginning of uart_rx_buf.
 * Must be called with interrupts disabled (or from an ISR). */
static void uartx_rx_dma_start(void)
{
    dma_resource_t *rx_resource = &dma_resource_pools[UART_RX_DMA_RESOURCE_INDEX];
    if (rx_resource->base == NULL)
    {
        return;
    }
    rb_write_pos = 0;
    dma_mgr_set_chn_dst_addr(rx_resource,
                             core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)uart_rx_buf));
    dma_mgr_set_chn_transize(rx_resource, UART_RX_DMA_BUFFER_SIZE);
    dma_mgr_enable_channel(rx_resource);
}

/* Periodic flush. Runs regardless of line idle / buffer full, so the RX DMA
 * buffer never gets a chance to wrap while the main loop is busy with SWD. */
SDK_DECLARE_EXT_ISR_M(UART_FLUSH_TIMER_IRQ, uart_flush_timer_isr)
void uart_flush_timer_isr(void)
{
    if (!gptmr_check_status(UART_FLUSH_TIMER, GPTMR_CH_CMP_STAT_MASK(UART_FLUSH_TIMER_CH, 0)))
    {
        return;
    }
    gptmr_clear_status(UART_FLUSH_TIMER, GPTMR_CH_CMP_STAT_MASK(UART_FLUSH_TIMER_CH, 0));

    if (s_uart2_com_mode)
    {
        uint32_t level = disable_global_irq(CSR_MSTATUS_MIE_MASK);
        uartx_rx_flush_locked();
        restore_global_irq(level);
    }
}

static void uart_flush_timer_init(void)
{
    gptmr_channel_config_t cfg;
    uint32_t freq;
    uint32_t ticks;

    init_gptmr0_clock();
    gptmr_channel_get_default_config(UART_FLUSH_TIMER, &cfg);
    freq = clock_get_frequency(clock_gptmr0);
    ticks = freq / (1000000U / UART_FLUSH_INTERVAL_US);
    if (ticks == 0U)
    {
        ticks = 1U;
    }
    cfg.mode = gptmr_work_mode_no_capture;
    cfg.reload = ticks;
    cfg.cmp[0] = ticks;
    cfg.cmp[1] = 0U;
    (void)gptmr_channel_config(UART_FLUSH_TIMER, UART_FLUSH_TIMER_CH, &cfg, false);
    gptmr_enable_irq(UART_FLUSH_TIMER, GPTMR_CH_CMP_IRQ_MASK(UART_FLUSH_TIMER_CH, 0));
    gptmr_channel_reset_count(UART_FLUSH_TIMER, UART_FLUSH_TIMER_CH);
    gptmr_start_counter(UART_FLUSH_TIMER, UART_FLUSH_TIMER_CH);
    intc_m_enable_irq_with_priority(UART_FLUSH_TIMER_IRQ, 1);
}

static void dma_channel_tc_callback(DMA_Type *ptr, uint32_t channel, void *user_data)
{
    (void)ptr;
    (void)user_data;
    dma_resource_t *rx_resource = &dma_resource_pools[UART_RX_DMA_RESOURCE_INDEX];
    dma_resource_t *tx_resource = &dma_resource_pools[UART_TX_DMA_RESOURCE_INDEX];

    if (rx_resource->channel == channel)
    {
        /* A full lap of the circular buffer completed: flush it. The DMA keeps
         * running (infinite loop), so no restart is required. */
        uint32_t level = disable_global_irq(CSR_MSTATUS_MIE_MASK);
        uartx_rx_flush_locked();
        restore_global_irq(level);
    }
    else if (tx_resource->channel == channel)
    {
        chry_dap_usb2uart_uart_send_complete(g_uart_tx_transfer_length);
    }
}

void uart_isr(void)
{
    if (!uart_is_rxline_idle(UART_BASE))
    {
        return;
    }
    uart_clear_rxline_idle_flag(UART_BASE);

    /* Flush the partial buffer on an idle line so low-rate traffic does not
     * have to wait for a full lap. */
    uint32_t level = disable_global_irq(CSR_MSTATUS_MIE_MASK);
    uartx_rx_flush_locked();
    restore_global_irq(level);
}

SDK_DECLARE_EXT_ISR_M(UART_IRQ, uart_isr)

void usb2uart_handler(void)
{
    if (!s_uart2_com_mode)
    {
        return;
    }

    /* Polled flush: same as the idle ISR but called from the main loop so the
     * USB CDC IN transfers are started without waiting for the idle interrupt. */
    uint32_t level = disable_global_irq(CSR_MSTATUS_MIE_MASK);
    uartx_rx_flush_locked();
    restore_global_irq(level);
}

/* Mux PA08 (TXD) / PA09 (RXD) to the UART2 alternate function. */
static void uartx_mux_to_uart(void)
{
    HPM_IOC->PAD[PIN_UART_RX].FUNC_CTL = IOC_PA09_FUNC_CTL_UART2_RXD;
    HPM_IOC->PAD[PIN_UART_TX].FUNC_CTL = IOC_PA08_FUNC_CTL_UART2_TXD;
}

/* Switch PA08/PA09 to UART2 so the CDC COM port works.
 * Used when the DAP is in SWD mode, disconnected, or idle. */
void uartx_enter_com_mode(void)
{
    uint32_t level = disable_global_irq(CSR_MSTATUS_MIE_MASK);
    /* Always re-mux: PORT_SWD_SETUP() parks the pads as GPIO first, even when
     * the software flag already says COM mode. Re-selecting the same UART2
     * function is a no-op and must not touch the FIFOs, otherwise every
     * DAP_Connect() during an SWD session would discard the up-to-16 bytes
     * sitting in the UART RX FIFO. Only a real JTAG -> COM transition needs
     * the FIFOs cleared. */
    uartx_mux_to_uart();
    if (!s_uart2_com_mode)
    {
        uart_reset_rx_fifo(UART_BASE);
        uart_reset_tx_fifo(UART_BASE);
        s_uart2_com_mode = 1;
    }
    restore_global_irq(level);
}

/* Give PA08/PA09 back to the JTAG engine (TDI/TDO as FGPIO).
 * The CDC COM port stays enumerated but no longer carries data. */
void uartx_enter_jtag_mode(void)
{
    uint32_t level = disable_global_irq(CSR_MSTATUS_MIE_MASK);
    /* Detach the pads from UART2 and hand them to the (F)GPIO path. The JTAG
     * code configures the GPIO direction/controller, but the pad function must
     * be cleared here, otherwise the UART2 alternate function stays selected. */
    HPM_IOC->PAD[PIN_UART_RX].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[PIN_UART_TX].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    s_uart2_com_mode = 0;
    uart_reset_rx_fifo(UART_BASE);
    uart_reset_tx_fifo(UART_BASE);
    restore_global_irq(level);
}

void uartx_io_init(void)
{
    PIN_UART_DTR = IOC_PAD_PA06;
    PIN_UART_RTS = IOC_PAD_PA07;

    /* Default state: UART2 owns PA08/PA09. */
    uartx_mux_to_uart();
    s_uart2_com_mode = 1;

#if UART2_DRIVE_DTR_RTS
    HPM_IOC->PAD[PIN_UART_DTR].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);
    HPM_IOC->PAD[PIN_UART_RTS].FUNC_CTL = IOC_PAD_FUNC_CTL_ALT_SELECT_SET(0);

    gpiom_set_pin_controller(HPM_GPIOM, GPIO_GET_PORT_INDEX(PIN_UART_DTR), GPIO_GET_PIN_INDEX(PIN_UART_DTR),
                             gpiom_soc_gpio0);
    gpio_set_pin_output(HPM_GPIO0, GPIO_GET_PORT_INDEX(PIN_UART_DTR), GPIO_GET_PIN_INDEX(PIN_UART_DTR));

    gpiom_set_pin_controller(HPM_GPIOM, GPIO_GET_PORT_INDEX(PIN_UART_RTS), GPIO_GET_PIN_INDEX(PIN_UART_RTS),
                             gpiom_soc_gpio0);
    gpio_set_pin_output(HPM_GPIO0, GPIO_GET_PORT_INDEX(PIN_UART_RTS), GPIO_GET_PIN_INDEX(PIN_UART_RTS));

    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(PIN_UART_DTR), GPIO_GET_PIN_INDEX(PIN_UART_DTR), 0);
    gpio_write_pin(HPM_GPIO0, GPIO_GET_PORT_INDEX(PIN_UART_RTS), GPIO_GET_PIN_INDEX(PIN_UART_RTS), 0);
#else
    (void)PIN_UART_DTR;
    (void)PIN_UART_RTS;
#endif
}

void uartx_preinit(void)
{
    /* The DMA manager must be initialised before any channel is requested,
     * otherwise dma_mgr_request_resource() always fails. */
    dma_mgr_init();

    uartx_io_init();

    /* 720 MHz PLL0CLK0 / 8 = 90 MHz UART2 clock (PLL0 is always initialised). */
    clock_set_source_divider(UART_CLK_NAME, clk_src_pll0_clk0, 8);
    clock_add_to_group(UART_CLK_NAME, 0);
    intc_m_enable_irq_with_priority(UART_IRQ, 2);
    uart_clear_rxline_idle_flag(UART_BASE);
    if (board_uart_dma_config() != status_success)
    {
        return;
    }
    uart_flush_timer_init();
}

/* Drop stale data and restart the RX DMA from the buffer start. */
static void uartx_rx_dma_restart(void)
{
    dma_resource_t *rx = &dma_resource_pools[UART_RX_DMA_RESOURCE_INDEX];
    if (rx->base == NULL)
    {
        return;
    }

    uint32_t level = disable_global_irq(CSR_MSTATUS_MIE_MASK);

    dma_mgr_disable_channel(rx);
    rb_write_pos = 0;
    chry_ringbuffer_reset(&g_uartrx);
    uartx_rx_dma_start();

    restore_global_irq(level);
}

/* Pick the achievable UART2 baud (src_freq / (div * osc), osc in 8..30 even,
 * div in 1..0xFFFF) that is closest to `baud`, clamped to UART2_MAX_BAUDRATE.
 * Unlike the SDK helper this never gives up on the 3% tolerance: it returns the
 * nearest representable rate so the COM port still works for e.g. 6M / 10M. */
static uint32_t uart2_round_baudrate(uint32_t src_freq, uint32_t baud)
{
    uint32_t best = 0;
    uint64_t best_err = (uint64_t)-1;

    if (baud > UART2_MAX_BAUDRATE)
    {
        baud = UART2_MAX_BAUDRATE;
    }
    if ((src_freq == 0) || (baud == 0))
    {
        return 0;
    }

    for (uint32_t osc = 8; osc <= 30; osc += 2)
    {
        uint64_t denom = (uint64_t)baud * osc;
        uint64_t div = ((uint64_t)src_freq + denom / 2) / denom; /* rounded */
        if (div < 1)
        {
            div = 1;
        }
        if (div > 0xFFFFU)
        {
            continue;
        }
        uint64_t actual = (uint64_t)src_freq / (div * osc);
        if ((actual == 0) || (actual > UART2_MAX_BAUDRATE))
        {
            continue;
        }
        uint64_t err = (actual > baud) ? (actual - baud) : (baud - actual);
        if (err < best_err)
        {
            best_err = err;
            best = (uint32_t)actual;
        }
    }
    return best;
}

void chry_dap_usb2uart_uart_config_callback(struct cdc_line_coding *line_coding)
{
    uart_config_t config = {0};
    uart_default_config(UART_BASE, &config);
    config.src_freq_in_hz = clock_get_frequency(UART_CLK_NAME);

    uint32_t requested = line_coding->dwDTERate;
    uint32_t applied = uart2_round_baudrate(config.src_freq_in_hz, requested);
    if (applied == 0)
    {
        applied = (requested > UART2_MAX_BAUDRATE) ? UART2_MAX_BAUDRATE : requested;
    }
    g_uart2_applied_baud = applied;
    if (applied != requested)
    {
        printf("uart2 baud: req %lu -> %lu (max %lu)\r\n",
               (unsigned long)requested, (unsigned long)applied, (unsigned long)UART2_MAX_BAUDRATE);
    }
    config.baudrate = applied;

    config.parity = line_coding->bParityType;
    config.word_length = line_coding->bDataBits - 5;
    config.num_of_stop_bits = line_coding->bCharFormat;
    config.fifo_enable = true;
    config.dma_enable = true;
    config.rx_fifo_level = uart_rx_fifo_trg_not_empty; /* this config should not change */
    config.tx_fifo_level = uart_tx_fifo_trg_not_full;
    config.rxidle_config.detect_enable = true;
    config.rxidle_config.detect_irq_enable = true;
    config.rxidle_config.idle_cond = uart_rxline_idle_cond_state_machine_idle;
    config.rxidle_config.threshold = 30U; /* 20bit */
    uart_init(UART_BASE, &config);
    uart_clear_rxline_idle_flag(UART_BASE);
    uart_reset_rx_fifo(UART_BASE);
    uart_reset_tx_fifo(UART_BASE);

    uartx_rx_dma_restart();
}

void chry_dap_usb2uart_uart_send_bydma(uint8_t *data, uint16_t len)
{
    uint32_t buf_addr;
    dma_resource_t *tx_resource = &dma_resource_pools[UART_TX_DMA_RESOURCE_INDEX];
    if ((len <= 0) || (tx_resource->base == NULL))
    {
        return;
    }
    g_uart_tx_transfer_length = len;
    buf_addr = core_local_mem_to_sys_address(HPM_CORE0, (uint32_t)data);
    dma_mgr_set_chn_src_addr(tx_resource, buf_addr);
    dma_mgr_set_chn_transize(tx_resource, len);
    dma_mgr_enable_channel(tx_resource);
}

static hpm_stat_t board_uart_dma_config(void)
{
    dma_mgr_chn_conf_t chg_config;
    dma_resource_t *resource = NULL;
    dma_mgr_get_default_chn_config(&chg_config);
    chg_config.src_width = DMA_MGR_TRANSFER_WIDTH_BYTE;
    chg_config.dst_width = DMA_MGR_TRANSFER_WIDTH_BYTE;
    /* uart rx dma config: single buffer, restarted on idle/TC */
    resource = &dma_resource_pools[UART_RX_DMA_RESOURCE_INDEX];
    if (dma_mgr_request_resource(resource) == status_success)
    {
        chg_config.src_mode = DMA_MGR_HANDSHAKE_MODE_HANDSHAKE;
        chg_config.src_addr_ctrl = DMA_ADDRESS_CONTROL_FIXED;
        chg_config.src_addr = (uint32_t)&UART_BASE->RBR;
        chg_config.dst_mode = DMA_MGR_HANDSHAKE_MODE_NORMAL;
        chg_config.dst_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_INCREMENT;
        chg_config.dst_addr = core_local_mem_to_sys_address(BOARD_RUNNING_CORE, (uint32_t)uart_rx_buf);
        chg_config.size_in_byte = UART_RX_DMA_BUFFER_SIZE;
        chg_config.en_dmamux = true;
        chg_config.dmamux_src = UART_RX_DMA;
        chg_config.linked_ptr = (uint32_t)NULL;
        chg_config.interrupt_mask = 0; /* terminal-count interrupt enabled */
        chg_config.en_infiniteloop = true; /* DMAV2 circular buffer */
        dma_mgr_setup_channel(resource, &chg_config);
        dma_mgr_install_chn_tc_callback(resource, dma_channel_tc_callback, NULL);
        dma_mgr_enable_chn_irq(resource, DMA_MGR_INTERRUPT_MASK_TC);
        dma_mgr_enable_dma_irq_with_priority(resource, 1);
        uartx_rx_dma_start();
    }
    /* uart tx dma config */
    resource = &dma_resource_pools[UART_TX_DMA_RESOURCE_INDEX];
    if (dma_mgr_request_resource(resource) == status_success)
    {
        chg_config.src_mode = DMA_MGR_HANDSHAKE_MODE_NORMAL;
        chg_config.src_addr_ctrl = DMA_MGR_ADDRESS_CONTROL_INCREMENT;
        chg_config.dst_mode = DMA_MGR_HANDSHAKE_MODE_HANDSHAKE;
        chg_config.dst_addr_ctrl = DMA_ADDRESS_CONTROL_FIXED;
        chg_config.dst_addr = (uint32_t)&UART_BASE->THR;
        chg_config.en_dmamux = true;
        chg_config.dmamux_src = UART_TX_DMA;
        chg_config.linked_ptr = (uint32_t)NULL;
        chg_config.en_infiniteloop = false;
        dma_mgr_setup_channel(resource, &chg_config);
        dma_mgr_install_chn_tc_callback(resource, dma_channel_tc_callback, NULL);
        dma_mgr_enable_chn_irq(resource, DMA_MGR_INTERRUPT_MASK_TC);
        dma_mgr_enable_dma_irq_with_priority(resource, 1);
    }
    return status_success;
}
