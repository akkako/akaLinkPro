#include "DAP_Port.h"
#include "DAP.h"
#include "drv_spi.h"

static inline uint8_t GetParity (uint32_t data) {
    data ^= data >> 16;
    data ^= data >> 8;
    data ^= data >> 4;
    data &= 0x0F;
    return (0x6996 >> data) & 1;
}

#define PIN_DELAY() PIN_DELAY_FAST()

static inline void SW_CLOCK_CYCLE() {
    PIN_SWCLK_TCK_CLR();
    PIN_DELAY();
    PIN_SWCLK_TCK_SET();
    PIN_DELAY();
}

static inline void SW_WRITE_BIT (uint32_t bit) {
    PIN_SWDIO_OUT (bit);
    PIN_SWCLK_TCK_CLR();
    PIN_DELAY();
    PIN_SWCLK_TCK_SET();
    PIN_DELAY();
}

static inline uint32_t SW_READ_BIT() {
    uint32_t bit;
    PIN_SWCLK_TCK_CLR();
    PIN_DELAY();
    bit = PIN_SWDIO_IN();
    PIN_SWCLK_TCK_SET();
    PIN_DELAY();
    return bit;
}

// Generate SWJ Sequence
//   count:  sequence bit count
//   data:   pointer to sequence bit data
//   return: none
void SWJ_Sequence_GPIO_Fast (uint32_t count, const uint8_t *data) {
    uint32_t val;
    uint32_t n;

    val = 0U;
    n = 0U;
    while (count--) {
        if (n == 0U) {
            val = *data++;
            n = 8U;
        }
        SW_WRITE_BIT (val);
        val >>= 1;
        n--;
    }
}

// Generate SWD Sequence
//   info:   sequence information
//   swdo:   pointer to SWDIO generated data
//   swdi:   pointer to SWDIO captured data
//   return: none
void SWD_Sequence_GPIO_Fast (uint32_t info, const uint8_t *swdo, uint8_t *swdi) {
    uint32_t val;
    uint32_t bit;
    uint32_t n, k;

    n = info & SWD_SEQUENCE_CLK;
    if (n == 0U) {
        n = 64U;
    }

    if (info & SWD_SEQUENCE_DIN) {
        while (n) {
            val = 0U;
            for (k = 8U; k && n; k--, n--) {
                bit = SW_READ_BIT();
                val >>= 1;
                val |= bit << 7;
            }
            val >>= k;
            *swdi++ = (uint8_t)val;
        }
    } else {
        while (n) {
            val = *swdo++;
            for (k = 8U; k && n; k--, n--) {
                SW_WRITE_BIT (val);
                val >>= 1;
            }
        }
    }
}

#if 0
uint8_t SWD_Read_GPIO_Fast(uint8_t header, uint32_t *data)
{
    uint32_t ack;
    uint32_t bit;
    uint8_t parity;
    uint8_t turn = DAP_Data.swd_conf.turnaround;
    uint8_t n;

    uint32_t val = 0;

    /* 发送 8 bit 包头 */
    for (uint8_t i = 8; i; i--)
    {
        SW_WRITE_BIT(header);
        header >>= 1;
    }
    drv_spi_gpio_mux_gpio_in();

    /* 方向转换 */
    PIN_SWDIR_INPUT();

    for (uint8_t n = turn; n; n--)
    {
        SW_CLOCK_CYCLE();
    }

    /* 读取目标 ACK */
    bit = SW_READ_BIT();
    ack = bit << 0;
    bit = SW_READ_BIT();
    ack |= bit << 1;
    bit = SW_READ_BIT();
    ack |= bit << 2;

    if (ack == DAP_TRANSFER_OK)
    {
        /* 读数据 */
        for (n = 32U; n; n--)
        {
            bit = SW_READ_BIT(); /* Read RDATA[0:31] */
            val >>= 1;
            val |= bit << 31;
        }
        parity = GetParity(val);

        /* 读校验位 */
        bit = SW_READ_BIT();

        if ((parity ^ bit) & 1U)
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
#else

#define SEND_HEAD_BIT()     \
    PIN_SWDIO_OUT (header); \
    PIN_SWCLK_TCK_CLR();    \
    header >>= 1;           \
    PIN_SWCLK_TCK_SET()

#define READ_DATA_BIT()        \
    PIN_SWCLK_TCK_CLR();       \
    bit = PIN_SWDIO_TMS_IN2(); \
    PIN_SWCLK_TCK_SET();       \
    val >>= 1;                 \
    val |= bit

#define SEND_DATA_BIT()  \
    PIN_SWCLK_TCK_CLR(); \
    PIN_SWDIO_OUT (val); \
    PIN_SWCLK_TCK_SET(); \
    val >>= 1

#define REPEAT_8(a) \
    a;              \
    a;              \
    a;              \
    a;              \
    a;              \
    a;              \
    a;              \
    a
#define REPEAT_32(a) \
    REPEAT_8 (a);    \
    REPEAT_8 (a);    \
    REPEAT_8 (a);    \
    REPEAT_8 (a)

uint8_t SWD_Read_GPIO_Fast1 (uint8_t header, uint8_t turnaround, uint8_t data_phase, uint8_t idle_cycles, uint32_t *data) {
    uint32_t ack;
    uint32_t bit;
    uint8_t parity;
    uint8_t turn = turnaround;
    uint8_t n;

    uint32_t val = 0;

    /* 发送 8 bit 包头 */
    REPEAT_8 (SEND_HEAD_BIT());

    drv_spi_gpio_mux_gpio_in();

    /* 方向转换 */
    PIN_SWDIR_INPUT();

    for (uint8_t n = turn; n; n--) {
        SW_CLOCK_CYCLE();
    }

    /* 读取目标 ACK */
    PIN_SWCLK_TCK_CLR();
    PIN_DELAY();
    bit = PIN_SWDIO_IN();
    PIN_SWCLK_TCK_SET();
    ack = bit << 0;

    PIN_SWCLK_TCK_CLR();
    PIN_DELAY();
    bit = PIN_SWDIO_IN();
    PIN_SWCLK_TCK_SET();
    ack |= bit << 1;

    PIN_SWCLK_TCK_CLR();
    PIN_DELAY();
    bit = PIN_SWDIO_IN();
    PIN_SWCLK_TCK_SET();
    ack |= bit << 2;

    if (ack == DAP_TRANSFER_OK) {
        /* 读数据 */
        REPEAT_32 (READ_DATA_BIT());

        /* 读校验位 */
        PIN_SWCLK_TCK_CLR();
        parity = GetParity (val);
        bit = PIN_SWDIO_IN();
        PIN_SWCLK_TCK_SET();

        if ((parity ^ bit) & 1U) {
            ack = DAP_TRANSFER_ERROR;
        }
        *data = val;

        /* 方向调转 */
        for (n = turn; n; n--) {
            SW_CLOCK_CYCLE();
        }
        // PIN_SWDIO_OUT_ENABLE();
        drv_spi_gpio_mux_gpio_out();
        PIN_SWDIR_OUTPUT();

        /* 传输空闲时钟 */
        uint8_t n = idle_cycles;
        if (n) {
            PIN_SWDIO_OUT (0U);
            for (; n; n--) {
                SW_CLOCK_CYCLE();
            }
        }

        /* SWDIO 输出高电平 */
        PIN_SWDIO_OUT (1U);
        return ((uint8_t)ack);
    }

    // 回复 WAIT 或者 FAULT
    else if ((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT)) {

        /* WAIT or FAULT response */
        if (data_phase) {
            for (uint8_t n = 32U + 1U; n; n--) {
                SW_CLOCK_CYCLE(); /* Dummy Read RDATA[0:31] + Parity */
            }
        }
        /* Turnaround */
        for (uint8_t n = turn; n; n--) {
            SW_CLOCK_CYCLE();
        }
        PIN_SWDIO_OUT_ENABLE();
        PIN_SWDIO_OUT (1U);
        PIN_SWDIR_OUTPUT();
        return ((uint8_t)ack);
    } else {
        /* Protocol error */
        for (uint8_t n = turn + 32U + 1U; n; n--) {
            SW_CLOCK_CYCLE(); /* Back off data phase */
        }
        PIN_SWDIO_OUT_ENABLE();
        PIN_SWDIO_OUT (1U);
        PIN_SWDIR_OUTPUT();
        return ((uint8_t)ack);
    }

    return ((uint8_t)ack);
}
#endif

#if 0
uint8_t SWD_Write_GPIO_Fast(uint8_t header, uint32_t *data)
{
    uint32_t ack;
    uint32_t bit;
    uint32_t val;
    uint8_t parity;
    uint8_t turn = DAP_Data.swd_conf.turnaround;
    uint8_t n;

    /* 发送 8 bit 包头 */
    for (uint8_t i = 8; i; i--)
    {
        SW_WRITE_BIT(header);
        header >>= 1;
    }
    drv_spi_gpio_mux_gpio_in();

    /* 方向转换 */
    PIN_SWDIR_INPUT();

    for (n = turn; n; n--)
    {
        SW_CLOCK_CYCLE();
    }

    /* 读取目标 ACK */
    bit = SW_READ_BIT();
    ack = bit << 0;
    bit = SW_READ_BIT();
    ack |= bit << 1;
    bit = SW_READ_BIT();
    ack |= bit << 2;

    if (ack == DAP_TRANSFER_OK)
    {
        /* 方向调转 */
        for (n = turn; n; n--)
        {
            SW_CLOCK_CYCLE();
        }

        /* 写 32 位数据 */
        drv_spi_gpio_mux_gpio_out();
        PIN_SWDIR_OUTPUT();
        val = *data;
        parity = GetParity(val);
        
        for (n = 32U; n; n--) {                       
            SW_WRITE_BIT(val); /* Write WDATA[0:31] */                      
            val >>= 1;                                
        } 
        /* 写校验位 */
        SW_WRITE_BIT(parity);

        /* 传输空闲时钟 */
        n = DAP_Data.transfer.idle_cycles;
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
}
uint8_t SWD_Write_GPIO_Fast1(uint8_t header, uint32_t *data)
{
    uint32_t ack;
    uint32_t bit;
    uint32_t val;
    uint8_t parity;
    uint8_t turn = DAP_Data.swd_conf.turnaround;
    uint8_t n;

    /* 发送 8 bit 包头 */
    REPEAT_8(SEND_HEAD_BIT());

    drv_spi_gpio_mux_gpio_in();

    /* 方向转换 */
    PIN_SWDIR_INPUT();

    for (n = turn; n; n--)
    {
        SW_CLOCK_CYCLE();
    }

    /* 读取目标 ACK */
    PIN_SWCLK_TCK_CLR();
    PIN_DELAY();
    bit = PIN_SWDIO_IN();
    PIN_SWCLK_TCK_SET();
    ack = bit << 0;

    PIN_SWCLK_TCK_CLR();
    PIN_DELAY();
    bit = PIN_SWDIO_IN();
    PIN_SWCLK_TCK_SET();
    ack |= bit << 1;

    PIN_SWCLK_TCK_CLR();
    PIN_DELAY();
    bit = PIN_SWDIO_IN();
    PIN_SWCLK_TCK_SET();
    ack |= bit << 2;

    if (ack == DAP_TRANSFER_OK)
    {
        /* 方向调转 */
        for (n = turn; n; n--)
        {
            SW_CLOCK_CYCLE();
        }

        /* 写 32 位数据 */
        drv_spi_gpio_mux_gpio_out();
        PIN_SWDIR_OUTPUT();
        val = *data;
        parity = GetParity(val);

        REPEAT_32(SEND_DATA_BIT());

        /* 写校验位 */
        // SW_WRITE_BIT(parity);
        PIN_SWDIO_OUT(parity);
        PIN_SWCLK_TCK_CLR();
        n = DAP_Data.transfer.idle_cycles;
        PIN_SWCLK_TCK_SET();

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
}
#else


uint8_t SWD_Write_GPIO_Fast1 (uint8_t header, uint8_t turnaround, uint8_t data_phase, uint8_t idle_cycles, uint32_t *data) {
    uint32_t ack;
    uint32_t bit;
    uint32_t val;
    uint8_t parity;
    uint8_t n;

    /* 发送 8 bit 包头 */
    REPEAT_8 (SEND_HEAD_BIT());

    drv_spi_gpio_mux_gpio_in();

    /* 方向转换 */
    PIN_SWDIR_INPUT();

    for (n = turnaround; n; n--) {
        SW_CLOCK_CYCLE();
    }

    /* 读取目标 ACK */
    PIN_SWCLK_TCK_CLR();
    PIN_DELAY();
    bit = PIN_SWDIO_IN();
    PIN_SWCLK_TCK_SET();
    ack = bit << 0;

    PIN_SWCLK_TCK_CLR();
    PIN_DELAY();
    bit = PIN_SWDIO_IN();
    PIN_SWCLK_TCK_SET();
    ack |= bit << 1;

    PIN_SWCLK_TCK_CLR();
    PIN_DELAY();
    bit = PIN_SWDIO_IN();
    PIN_SWCLK_TCK_SET();
    ack |= bit << 2;

    /* 方向调转 */
    for (n = turnaround; n; n--) {
        SW_CLOCK_CYCLE();
    }

    drv_spi_gpio_mux_gpio_out();
    PIN_SWDIR_OUTPUT();

    if (ack == DAP_TRANSFER_OK) {
        /* 写 32 位数据 */

        val = *data;
        parity = GetParity (val);

        REPEAT_32 (SEND_DATA_BIT());

        /* 写校验位 */
        // SW_WRITE_BIT(parity);
        PIN_SWDIO_OUT (parity);
        PIN_SWCLK_TCK_CLR();
        n = idle_cycles;
        PIN_SWCLK_TCK_SET();

        /* 传输空闲时钟 */
        if (n) {
            PIN_SWDIO_OUT (0U);
            for (; n; n--) {
                SW_CLOCK_CYCLE();
            }
        }
    } else if ((ack == DAP_TRANSFER_WAIT) || (ack == DAP_TRANSFER_FAULT)) {
        /* WAIT or FAULT response */
        if (data_phase) {
            PIN_SWDIO_OUT (0U);
            for (n = 32U + 1U; n; n--) {
                SW_CLOCK_CYCLE(); /* Dummy Write WDATA[0:31] + Parity */
            }
        }
    } else {
        /* Protocol error */
        for (n = 32U + 1U; n; n--) {
            SW_CLOCK_CYCLE(); /* Back off data phase */
        }
    }

    PIN_SWDIO_OUT (1U);
    return ((uint8_t)ack);
}
#endif