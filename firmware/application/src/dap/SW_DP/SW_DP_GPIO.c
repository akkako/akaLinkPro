#include "DAP_Port.h"
#include "DAP.h"
#include "drv_spi.h"

static uint32_t dummy = 0xFFFFFFFF;

static inline uint8_t GetParity(uint32_t data)
{
    data ^= data >> 16;
    data ^= data >> 8;
    data ^= data >> 4;
    data &= 0x0F;
    return (0x6996 >> data) & 1;
}

#define PIN_DELAY() PIN_DELAY_FAST()

static inline void SW_CLOCK_CYCLE()
{
    PIN_SWCLK_TCK_CLR();
    PIN_DELAY();
    PIN_SWCLK_TCK_SET();
    // PIN_DELAY();
}

static inline void SW_WRITE_BIT(uint32_t bit)
{
    PIN_SWDIO_OUT(bit);
    PIN_SWCLK_TCK_CLR();
    PIN_DELAY();
    PIN_SWCLK_TCK_SET();
    // PIN_DELAY();
}

static inline uint32_t SW_READ_BIT()
{
    uint32_t bit;
    PIN_SWCLK_TCK_CLR();
    PIN_DELAY();
    bit = PIN_SWDIO_IN();
    PIN_SWCLK_TCK_SET();
    // PIN_DELAY();
    return bit;
}

static inline uint32_t SW_READ_BIT_OPT()
{
    uint32_t bit;
    PIN_SWCLK_TCK_CLR();
    bit = PIN_SWDIO_IN();
    PIN_SWCLK_TCK_SET();
    // PIN_DELAY();
    return bit;
}

// Generate SWJ Sequence
//   count:  sequence bit count
//   data:   pointer to sequence bit data
//   return: none
void SWJ_Sequence_GPIO(uint32_t count, const uint8_t *data)
{
    uint32_t val;
    uint32_t n;
    uint32_t spi_transmit_bytes = 0;
    uint32_t spi_transmit_remain_bits = 0;

    spi_transmit_remain_bits = count % 8;

    if (count > 8)
    {
        spi_transmit_bytes = count / 8;
        drv_spi_gpio_mux_spi();
        drv_spi_dma_tx_preset((uint8_t *)data, spi_transmit_bytes);
        drv_spi_dma_tx_start();
        drv_spi_dma_wait_tx();
        drv_spi_gpio_mux_gpio_out();
    }

    count = spi_transmit_remain_bits;
    data += spi_transmit_bytes;

    val = 0U;
    n = 0U;
    while (count--)
    {
        if (n == 0U)
        {
            val = *data++;
            n = 8U;
        }
        if (val & 1U)
        {
            PIN_SWDIO_TMS_SET();
        }
        else
        {
            PIN_SWDIO_TMS_CLR();
        }
        SW_CLOCK_CYCLE();
        val >>= 1;
        n--;
    }
}

// Generate SWD Sequence
//   info:   sequence information
//   swdo:   pointer to SWDIO generated data
//   swdi:   pointer to SWDIO captured data
//   return: none
void SWD_Sequence_GPIO(uint32_t info, const uint8_t *swdo, uint8_t *swdi)
{
    uint32_t val;
    uint32_t bit;
    uint32_t n, k;

    n = info & SWD_SEQUENCE_CLK;
    if (n == 0U)
    {
        n = 64U;
    }

    if (info & SWD_SEQUENCE_DIN)
    {
        while (n)
        {
            val = 0U;
            for (k = 8U; k && n; k--, n--)
            {
                bit = SW_READ_BIT();
                val >>= 1;
                val |= bit << 7;
            }
            val >>= k;
            *swdi++ = (uint8_t)val;
        }
    }
    else
    {
        while (n)
        {
            val = *swdo++;
            for (k = 8U; k && n; k--, n--)
            {
                SW_WRITE_BIT(val);
                val >>= 1;
            }
        }
    }
}

uint8_t SWD_Read_GPIO(uint8_t header, uint32_t *data)
{
    register uint32_t ack;
    register uint32_t ack1;
    register uint32_t ack2;
    register uint8_t parity;
    register uint8_t turn = DAP_Data.swd_conf.turnaround;
    register uint8_t n;

    uint32_t val = 0;

    /* 发送 8 bit 包头 */
    drv_spi_gpio_mux_spi();
    drv_spi_tx(header);
    drv_spi_dma_rx_preset((uint8_t *)&dummy, (uint8_t *)&val, 4);
    PIN_SWCLK_TCK_CLR();
    drv_spi_tx_wait();
    drv_spi_gpio_mux_gpio_in();

    /* 方向转换 */
    PIN_SWDIR_INPUT();
    PIN_SWCLK_TCK_SET();

    for (uint8_t n = turn - 1; n; n--)
    {
        SW_CLOCK_CYCLE();
    }

    /* 读取目标 ACK */
    ack = SW_READ_BIT_OPT();
    ack1 = SW_READ_BIT_OPT();
    ack2 = SW_READ_BIT_OPT();
    ack = (ack2 << 2) | (ack1 << 1) | (ack);

    if (ack == DAP_TRANSFER_OK)
    {
        /* 读数据 */
        drv_spi_gpio_mux_spi();
        drv_spi_dma_rx_start();
        drv_spi_dma_wait_rx();
        drv_spi_gpio_mux_gpio_in();
        parity = GetParity(val);

        /* 读校验位 */
        ack1 = SW_READ_BIT_OPT();

        if ((parity ^ ack1) & 1U)
        {
            ack = DAP_TRANSFER_ERROR;
        }
        *data = val;

        /* 方向调转 */
        for (n = turn; n; n--)
        {
            SW_CLOCK_CYCLE();
        }
        // PIN_SWDIO_OUT_ENABLE();
        drv_spi_gpio_mux_gpio_out();
        PIN_SWDIR_OUTPUT();

        /* 传输空闲时钟 */
        uint8_t n = DAP_Data.transfer.idle_cycles;
        if (n)
        {
            PIN_SWDIO_OUT(0U);
            for (; n; n--)
            {
                SW_CLOCK_CYCLE();
            }
        }

        /* SWDIO 输出高电平 */
        PIN_SWDIO_OUT(1U);
        return ((uint8_t)ack);
    }

    // 回复 WAIT 或者 FAULT
    else if ((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT))
    {

        /* WAIT or FAULT response */
        if (DAP_Data.swd_conf.data_phase)
        {
            for (uint8_t n = 32U + 1U; n; n--)
            {
                SW_CLOCK_CYCLE(); /* Dummy Read RDATA[0:31] + Parity */
            }
        }
        /* Turnaround */
        for (uint8_t n = turn; n; n--)
        {
            SW_CLOCK_CYCLE();
        }
        PIN_SWDIO_OUT_ENABLE();
        PIN_SWDIO_OUT(1U);
        PIN_SWDIR_OUTPUT();
        return ((uint8_t)ack);
    }
    else
    {
        /* Protocol error */
        for (uint8_t n = turn + 32U + 1U; n; n--)
        {
            SW_CLOCK_CYCLE(); /* Back off data phase */
        }
        PIN_SWDIO_OUT_ENABLE();
        PIN_SWDIO_OUT(1U);
        PIN_SWDIR_OUTPUT();
        return ((uint8_t)ack);
    }

    return ((uint8_t)ack);
}

uint8_t SWD_Write_GPIO(uint8_t header, uint32_t *data)
{
#if 0
    register uint32_t ack;
    register uint32_t ack1;
    register uint32_t ack2;
    register uint8_t parity;
    register uint8_t turn = DAP_Data.swd_conf.turnaround;
    register uint8_t n;

    /* 发送 8 bit 包头 */
    drv_spi_gpio_mux_spi();
    drv_spi_tx(header);
    drv_spi_dma_tx_preset((uint8_t *)data, 4);
    PIN_SWCLK_TCK_CLR();
    drv_spi_tx_wait();
    drv_spi_gpio_mux_gpio_in();

    /* 方向转换 */
    PIN_SWDIR_INPUT();
    PIN_SWCLK_TCK_SET();

    for (n = turn - 1; n; n--)
    {
        SW_CLOCK_CYCLE();
    }

    /* 读取目标 ACK */
    ack = SW_READ_BIT_OPT();
    ack1 = SW_READ_BIT_OPT();
    ack2 = SW_READ_BIT_OPT();
    ack = (ack2 << 2) | (ack1 << 1) | (ack);

    if (ack == DAP_TRANSFER_OK)
    {
        /* 方向调转 */
        for (n = turn; n; n--)
        {
            SW_CLOCK_CYCLE();
        }

        /* 写 32 位数据 */
        drv_spi_gpio_mux_spi();
        PIN_SWDIR_OUTPUT();
        drv_spi_dma_tx_start();
        parity = GetParity(*data);
        n = DAP_Data.transfer.idle_cycles;
        drv_spi_dma_wait_tx();
        drv_spi_gpio_mux_gpio_out();
        /* 写校验位 */
        SW_WRITE_BIT(parity);

        /* 传输空闲时钟 */
        if (n)
        {
            PIN_SWDIO_OUT(0U);
            for (; n; n--)
            {
                SW_CLOCK_CYCLE();
            }
        }

        /* SWDIO 切换输出 */
        PIN_SWDIO_OUT(1U);
        return ((uint8_t)ack);
    }
    else if ((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT))
    {
        /* WAIT or FAULT response */
        for (n = turn; n; n--)
        {
            SW_CLOCK_CYCLE();
        }
        PIN_SWDIO_OUT_ENABLE();
        PIN_SWDIR_OUTPUT();
        if (DAP_Data.swd_conf.data_phase)
        {
            PIN_SWDIO_OUT(0U);
            for (n = 32U + 1U; n; n--)
            {
                SW_CLOCK_CYCLE(); /* Dummy Write WDATA[0:31] + Parity */
            }
        }
        PIN_SWDIO_OUT(1U);
        return ((uint8_t)ack);
    }
    else
    {
        /* Protocol error */
        for (n = turn + 32U + 1U; n; n--)
        {
            SW_CLOCK_CYCLE(); /* Back off data phase */
        }
        PIN_SWDIO_OUT_ENABLE();
        PIN_SWDIO_OUT(1U);
        PIN_SWDIR_OUTPUT();
        return ((uint8_t)ack);
    }
#else
    return 0;
#endif
}
