#include <stdio.h>
#include <hpm_gpiom_soc_drv.h>
#include <hpm_gpio_drv.h>
#include <hpm_gpiom_drv.h>
#include "board.h"
#include "hpm_uart_drv.h"
#include "hpm_debug_console.h"
#include "hpm_dma_mgr.h"
#include "hpm_sysctl_drv.h"
#include "usb_composite.h"
#include "cdc_interface.h"

#define UART_BASE HPM_UART2
#define UART_IRQ IRQn_UART2
#define UART_CLK_NAME clock_uart2
#define UART_RX_DMA HPM_DMA_SRC_UART2_RX
#define UART_RX_DMA_RESOURCE_INDEX (0U)
/* Single RX buffer, restarted on idle/TC. Keeps the "current write position"
 * unambiguous (a linked-descriptor ring has a race between the hardware and
 * the software descriptor index). */
#define UART_RX_DMA_BUFFER_SIZE (4096U)

#define UART_TX_DMA HPM_DMA_SRC_UART2_TX
#define UART_TX_DMA_RESOURCE_INDEX (1U)
// #define UART_TX_DMA_BUFFER_SIZE    (8192U)

/* PA06/PA07 are not connected to modem-control nets on this board, so do not
 * drive them by default. Set to 1 if a future board uses DTR/RTS. */
#ifndef UART2_DRIVE_DTR_RTS
#define UART2_DRIVE_DTR_RTS (0)
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

static hpm_stat_t board_uart_dma_config(void);
static void uartx_mux_to_uart(void);
static void uartx_rx_dma_start(void);

static uint32_t PIN_UART_DTR = 0;
static uint32_t PIN_UART_RTS = 0;

/* Copy the bytes received so far in the RX buffer into the ringbuffer.
 * Must be called with interrupts disabled (or from an ISR). */
static void uartx_rx_flush_locked(void)
{
    dma_resource_t *rx_resource = &dma_resource_pools[UART_RX_DMA_RESOURCE_INDEX];
    if (rx_resource->base == NULL)
    {
        return;
    }
    uint32_t remaining = dma_get_remaining_transfer_size(rx_resource->base, rx_resource->channel);

    if (remaining > UART_RX_DMA_BUFFER_SIZE)
    {
        return;
    }

    uint32_t received = UART_RX_DMA_BUFFER_SIZE - remaining;
    if (received > rb_write_pos)
    {
        chry_ringbuffer_write(&g_uartrx, &uart_rx_buf[rb_write_pos], received - rb_write_pos);
        rb_write_pos = received;
    }
}

/* (Re)start the single-buffer RX DMA from the beginning of uart_rx_buf.
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

/* Flush the bytes received so far and restart the RX DMA. Used when the line
 * goes idle and when a full buffer has been received. */
static void uartx_rx_flush_and_restart(void)
{
    dma_resource_t *rx_resource = &dma_resource_pools[UART_RX_DMA_RESOURCE_INDEX];
    if (rx_resource->base == NULL)
    {
        return;
    }
    /* Stop first so no byte can slip in between the flush and the restart. */
    dma_mgr_disable_channel(rx_resource);
    uartx_rx_flush_locked();
    uartx_rx_dma_start();
}

static void dma_channel_tc_callback(DMA_Type *ptr, uint32_t channel, void *user_data)
{
    (void)ptr;
    (void)user_data;
    dma_resource_t *rx_resource = &dma_resource_pools[UART_RX_DMA_RESOURCE_INDEX];
    dma_resource_t *tx_resource = &dma_resource_pools[UART_TX_DMA_RESOURCE_INDEX];

    if (rx_resource->channel == channel)
    {
        /* RX buffer full: flush everything and restart from the buffer start. */
        uartx_rx_flush_and_restart();
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
     * have to wait for the buffer to fill up, then restart the RX DMA. */
    uint32_t level = disable_global_irq(CSR_MSTATUS_MIE_MASK);
    uartx_rx_flush_and_restart();
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
    /* Always re-mux: the JTAG/SWD setup code may have switched the pads to
     * GPIO even while the software flag was already set. */
    uartx_mux_to_uart();
    uart_reset_rx_fifo(UART_BASE);
    uart_reset_tx_fifo(UART_BASE);
    s_uart2_com_mode = 1;
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

void chry_dap_usb2uart_uart_config_callback(struct cdc_line_coding *line_coding)
{
    uart_config_t config = {0};
    uart_default_config(UART_BASE, &config);
    config.baudrate = line_coding->dwDTERate;
    config.parity = line_coding->bParityType;
    config.word_length = line_coding->bDataBits - 5;
    config.num_of_stop_bits = line_coding->bCharFormat;
    config.fifo_enable = true;
    config.dma_enable = true;
    config.src_freq_in_hz = clock_get_frequency(UART_CLK_NAME);
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
        dma_mgr_setup_channel(resource, &chg_config);
        dma_mgr_install_chn_tc_callback(resource, dma_channel_tc_callback, NULL);
        dma_mgr_enable_chn_irq(resource, DMA_MGR_INTERRUPT_MASK_TC);
        dma_mgr_enable_dma_irq_with_priority(resource, 1);
    }
    return status_success;
}
