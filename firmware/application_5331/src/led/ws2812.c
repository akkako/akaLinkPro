//
// 纯 C 实现的 WS2812 LED 驱动，替代原 C++ 版本以剥离 libstdc++ 依赖。
//

#include "ws2812.h"
#include "usb_config.h"
#include "board.h"
#include "hpm_common.h"
#include "hpm_gpio_drv.h"
#include "hpm_spi.h"

/* 仅 1 颗 LED，使用静态缓冲区，避免 malloc/new */
#define PIXEL_COUNT 1u

/* SPI 模式下每个 bit 用一个字节表示（8MHz SPI，1bit = 1.25us） */
#define SPI_BIT0_PULSE 0xE0u /* 0b11100000  ~37.5% high */
#define SPI_BIT1_PULSE 0xF8u /* 0b11111000  ~62.5% high */
#define SPI_RESET_BYTES 75u  /* 全 0 复位码（>50us 低电平） */
#define SPI_BUF_LEN (PIXEL_COUNT * 3u * 8u + SPI_RESET_BYTES)


/* GRB 顺序存储，与原 NeoPixel::buffer 一致 */
static uint8_t s_grb[PIXEL_COUNT * 3];
/* SPI 模式编码缓冲区（末尾 75 字节恒为 0 作为复位码） */
static uint8_t s_spi_buf[SPI_BUF_LEN];

static void ws2812_spi_flush(void) {
    /* 将 GRB 字节流编码为 SPI 字节流（每 bit -> 1 byte），MSB first */
    for (uint32_t j = 0; j < (PIXEL_COUNT * 3u); j++) {
        uint8_t byte = s_grb[j];
        for (uint32_t i = 0; i < 8u; i++) {
            s_spi_buf[j * 8u + i] = (byte & 0x80u) ? SPI_BIT1_PULSE : SPI_BIT0_PULSE;
            byte <<= 1;
        }
    }
    /* 末尾 SPI_RESET_BYTES 已为 0（静态变量零初始化），作为复位码 */
    hpm_spi_transmit_blocking(HPM_SPI0, s_spi_buf, SPI_BUF_LEN, 0xFFFFu);
}

static void ws2812_spi_init(void) {
    HPM_IOC->PAD[IOC_PAD_PA07].FUNC_CTL = IOC_PA07_FUNC_CTL_SPI0_MOSI;

    spi_initialize_config_t init_config;
    hpm_spi_get_default_init_config(&init_config);
    init_config.direction = spi_msb_first;
    init_config.mode = spi_master_mode;
    init_config.clk_phase = spi_sclk_sampling_odd_clk_edges;
    init_config.clk_polarity = spi_sclk_low_idle;
    init_config.data_len = 8;
    if (hpm_spi_initialize(HPM_SPI0, &init_config) != status_success) {
        printf("SPI init failed!\r\n");
        while (1) {
        }
    }
    if (hpm_spi_set_sclk_frequency(HPM_SPI0, 8UL * 1000UL * 1000UL) != status_success) {
        printf("hpm_spi_set_sclk_frequency fail\n");
        while (1) {
        }
    }
}

void WS2812_Init(void) {
    ws2812_spi_init();
}

static void ws2812_set_pixel_rgb(uint8_t r, uint8_t g, uint8_t b) {
    /* NeoPixel buffer 采用 GRB 顺序 */
    s_grb[0] = g;
    s_grb[1] = r;
    s_grb[2] = b;
}

void WS2812_SetColor(uint32_t color) {
    ws2812_set_pixel_rgb((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
    ws2812_spi_flush();
}

void WS2812_ShowFadeOn(void) {
    static uint8_t j = 0;
    j++;
    ws2812_set_pixel_rgb(j, j, j);
    ws2812_spi_flush();
}

void WS2812_ShowRainbow(void) {
    static int j = 0;
    j++;
    uint8_t r, g, b;
    uint8_t pos = (uint8_t) (j & 255);
    if (pos < 85) {
        r = (uint8_t) (pos * 3);
        g = (uint8_t) (255 - pos * 3);
        b = 0;
    } else if (pos < 170) {
        pos -= 85;
        r = (uint8_t) (255 - pos * 3);
        g = 0;
        b = (uint8_t) (pos * 3);
    } else {
        pos -= 170;
        r = 0;
        g = (uint8_t) (pos * 3);
        b = (uint8_t) (255 - pos * 3);
    }
    r = (uint8_t) (r / 16);
    g = (uint8_t) (g / 16);
    b = (uint8_t) (b / 16);
    ws2812_set_pixel_rgb(r, g, b);
    ws2812_spi_flush();
}

void WS2812_TurnOff(void) {

    ws2812_set_pixel_rgb(0, 0, 0);
    ws2812_spi_flush();
}
