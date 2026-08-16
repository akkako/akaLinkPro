#include "DAP_config.h"
#include "DAP.h"
#include "hpm_common.h"

#define PIN_DELAY() PIN_DELAY_SLOW(DAP_Data.clock_delay)

// SW Macros

static inline uint8_t GetParity(uint32_t data)
{
    data ^= data >> 16;
    data ^= data >> 8;
    data ^= data >> 4;
    data &= 0x0F;
    return (0x6996 >> data) & 1;
}

#define PIN_SWCLK_SET PIN_SWCLK_TCK_SET
#define PIN_SWCLK_CLR PIN_SWCLK_TCK_CLR

static inline void SW_CLOCK_CYCLE(void)
{
    PIN_SWCLK_CLR();
    PIN_DELAY();
    PIN_SWCLK_SET();
    PIN_DELAY();
}

static inline void SW_WRITE_BIT(uint8_t bit)
{
    PIN_SWDIO_OUT(bit);
    PIN_SWCLK_CLR();
    PIN_DELAY();
    PIN_SWCLK_SET();
    PIN_DELAY();
}
static inline uint8_t SW_READ_BIT(void)
{
    uint8_t bit;
    PIN_SWCLK_CLR();
    PIN_DELAY();
    PIN_DELAY();
    PIN_DELAY();
    bit = PIN_SWDIO_IN();
    PIN_SWCLK_SET();
    PIN_DELAY();
    return bit;
}

#if ((DAP_SWD != 0) || (DAP_JTAG != 0))
/**
 * @brief SWJ Sequence GPIO Slow Implementation
 * @param count sequence bit count
 * @param data pointer to sequence bit data
 * @return none
 */
ATTR_RAMFUNC void SWJ_Sequence_GPIO_Slow(uint32_t count, const uint8_t *data)
{
    register uint8_t val;
    register uint32_t pack_bytes = count / 8;
    register uint32_t tail_bits = count % 8;

    for (uint32_t i = 0; i < pack_bytes; i++)
    {
        val = *data++;
        for (uint32_t j = 0; j < 8; j++)
        {
            SW_WRITE_BIT(val);
            val >>= 1;
        }
    }

    val = *data;

    for (uint32_t i = 0; i < tail_bits; i++)
    {
        SW_WRITE_BIT(val);
        val >>= 1;
    }
}
#endif

#if (DAP_SWD != 0)
/**
 * @brief SWD Sequence GPIO Slow Implementation
 * @param info sequence information
 * @param swdo pointer to SWDIO generated data
 * @param swdi pointer to SWDIO captured data
 * @return none
 */
ATTR_RAMFUNC void SWD_Sequence_GPIO_Slow(uint32_t info, const uint8_t *swdo, uint8_t *swdi)
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
#endif

#if (DAP_SWD != 0)

/**
 * @brief SWD Write GPIO Slow Implementation
 * @param request HEADER[7:0]
 * @param data DATA[31:0]
 * @return ACK[2:0]
 */
ATTR_RAMFUNC uint8_t SWD_Write_GPIO_Slow(uint8_t header, uint32_t *data)
{
    uint32_t ack;
    uint32_t bit;
    uint32_t val;
    uint32_t parity;

    uint32_t n;

    /* Packet Request */
    SW_WRITE_BIT(header);
    header >>= 1;
    SW_WRITE_BIT(header);
    header >>= 1;
    SW_WRITE_BIT(header);
    header >>= 1;
    SW_WRITE_BIT(header);
    header >>= 1;
    SW_WRITE_BIT(header);
    header >>= 1;
    SW_WRITE_BIT(header);
    header >>= 1;
    SW_WRITE_BIT(header);
    header >>= 1;
    SW_WRITE_BIT(header);

    /* Turnaround */
    PIN_SWDIO_OUT_DISABLE();
    for (n = DAP_Data.swd_conf.turnaround; n; n--)
    {
        SW_CLOCK_CYCLE();
    }

    /* Acknowledge response */
    bit = SW_READ_BIT();
    ack = bit << 0;
    bit = SW_READ_BIT();
    ack |= bit << 1;
    bit = SW_READ_BIT();
    ack |= bit << 2;

    if (ack == DAP_TRANSFER_OK)
    { /* OK response */
        /* Data transfer */
        /* Turnaround */
        for (n = DAP_Data.swd_conf.turnaround; n; n--)
        {
            SW_CLOCK_CYCLE();
        }
        PIN_SWDIO_OUT_ENABLE();
        /* Write data */
        val = *data;
        parity = GetParity(val);

        for (n = 32U; n; n--)
        {
            SW_WRITE_BIT(val); /* Write WDATA[0:31] */
            val >>= 1;
        }
        SW_WRITE_BIT(parity); /* Write Parity Bit */

        /* Capture Timestamp */
        // if (request & DAP_TRANSFER_TIMESTAMP)
        // {
        //     DAP_Data.timestamp = TIMESTAMP_GET();
        // }
        /* Idle cycles */
        n = DAP_Data.transfer.idle_cycles;
        if (n)
        {
            PIN_SWDIO_OUT(0U);
            for (; n; n--)
            {
                SW_CLOCK_CYCLE();
            }
        }
        PIN_SWDIO_OUT(1U);
        return ((uint8_t)ack);
    }

    if ((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT))
    {
        /* Turnaround */
        for (n = DAP_Data.swd_conf.turnaround; n; n--)
        {
            SW_CLOCK_CYCLE();
        }
        PIN_SWDIO_OUT_ENABLE();
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

    /* Protocol error */
    for (n = DAP_Data.swd_conf.turnaround + 32U + 1U; n; n--)
    {
        SW_CLOCK_CYCLE(); /* Back off data phase */
    }
    PIN_SWDIO_OUT_ENABLE();
    PIN_SWDIO_OUT(1U);
    return ((uint8_t)ack);
}

/**
 * @brief SWD Read GPIO Slow Implementation
 * @param header HEADER[7:0]
 * @param data DATA[31:0]
 * @return ACK[2:0]
 */
ATTR_RAMFUNC uint8_t SWD_Read_GPIO_Slow(uint8_t header, uint32_t *data)
{
    uint32_t ack;
    uint32_t bit;
    uint32_t val;
    uint32_t parity;

    uint32_t n;

    /* Packet Request */
    /* Packet Request */
    SW_WRITE_BIT(header);
    header >>= 1;
    SW_WRITE_BIT(header);
    header >>= 1;
    SW_WRITE_BIT(header);
    header >>= 1;
    SW_WRITE_BIT(header);
    header >>= 1;
    SW_WRITE_BIT(header);
    header >>= 1;
    SW_WRITE_BIT(header);
    header >>= 1;
    SW_WRITE_BIT(header);
    header >>= 1;
    SW_WRITE_BIT(header);

    /* Turnaround */
    PIN_SWDIO_OUT_DISABLE();
    for (n = DAP_Data.swd_conf.turnaround; n; n--)
    {
        SW_CLOCK_CYCLE();
    }

    /* Acknowledge response */
    bit = SW_READ_BIT();
    ack = bit << 0;
    bit = SW_READ_BIT();
    ack |= bit << 1;
    bit = SW_READ_BIT();
    ack |= bit << 2;

    if (ack == DAP_TRANSFER_OK)
    {
        /* OK response */
        /* Data transfer */
        /* Read data */
        val = 0U;
        for (n = 32U; n; n--)
        {
            bit = SW_READ_BIT(); /* Read RDATA[0:31] */
            val >>= 1;
            val |= bit << 31;
        }
        bit = SW_READ_BIT(); /* Read Parity */
        parity = GetParity(val);
        if ((parity ^ bit) & 1U)
        {
            ack = DAP_TRANSFER_ERROR;
        }
        if (data)
        {
            *data = val;
        }
        /* Turnaround */
        for (n = DAP_Data.swd_conf.turnaround; n; n--)
        {
            SW_CLOCK_CYCLE();
        }
        PIN_SWDIO_OUT_ENABLE();

        /* Capture Timestamp */
        // if (request & DAP_TRANSFER_TIMESTAMP)
        // {
        //     DAP_Data.timestamp = TIMESTAMP_GET();
        // }
        /* Idle cycles */
        n = DAP_Data.transfer.idle_cycles;
        if (n)
        {
            PIN_SWDIO_OUT(0U);
            for (; n; n--)
            {
                SW_CLOCK_CYCLE();
            }
        }
        PIN_SWDIO_OUT(1U);
        return ((uint8_t)ack);
    }

    if ((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT))
    {
        /* WAIT or FAULT response */
        if (DAP_Data.swd_conf.data_phase)
        {
            for (n = 32U + 1U; n; n--)
            {
                SW_CLOCK_CYCLE(); /* Dummy Read RDATA[0:31] + Parity */
            }
        }
        /* Turnaround */
        for (n = DAP_Data.swd_conf.turnaround; n; n--)
        {
            SW_CLOCK_CYCLE();
        }
        PIN_SWDIO_OUT_ENABLE();
        PIN_SWDIO_OUT(1U);
        return ((uint8_t)ack);
    }

    /* Protocol error */
    for (n = DAP_Data.swd_conf.turnaround + 32U + 1U; n; n--)
    {
        SW_CLOCK_CYCLE(); /* Back off data phase */
    }
    PIN_SWDIO_OUT_ENABLE();
    PIN_SWDIO_OUT(1U);
    return ((uint8_t)ack);
}

#endif /* (DAP_SWD != 0) */