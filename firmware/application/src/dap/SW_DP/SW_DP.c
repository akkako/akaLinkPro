#include "DAP_Port.h"
#include "DAP.h"
#include "drv_spi.h"

/**
 * @brief 产生 SWD 序列时序
 * @param info 时序参数，长度（位数）
 * @param swdo SWD 输出数据
 * @param swdi SWD 输入数据
 */
void SWD_Sequence (uint32_t info, const uint8_t *swdo, uint8_t *swdi) {
    if (DAP_Data.swd_spi_sim) {
        if (DAP_Data.fast_clock) {
            SWD_Sequence_SPI_Fast (info, swdo, swdi);
        } else {
            SWD_Sequence_SPI (info, swdo, swdi);
        }
    } else {
        if (DAP_Data.fast_clock) {
            SWD_Sequence_GPIO_Fast (info, swdo, swdi);
        } else {
            SWD_Sequence_GPIO (info, swdo, swdi);
        }
    }
}

/**
 * @brief 产生 SWJ 序列时序
 * @param count SWJ 序列长度（位数）
 * @param data SWJ 序列数据
 */
void SWJ_Sequence (uint32_t count, const uint8_t *data) {
    if (DAP_Data.swd_spi_sim) {
        if (DAP_Data.fast_clock) {
            SWJ_Sequence_SPI_Fast (count, data);
        } else {
            SWJ_Sequence_SPI (count, data);
        }
    } else {
        if (DAP_Data.fast_clock) {
            SWJ_Sequence_GPIO_Fast (count, data);
        } else {
            SWJ_Sequence_GPIO (count, data);
        }
    }
}

/**
 * @brief SWD 读操作
 * @param header SWD 头
 * @param data 读数据指针
 * @return ACK 值
 */
uint8_t SWD_Read (uint8_t header, uint32_t *data) {
    if (DAP_Data.swd_spi_sim) {
        if (DAP_Data.fast_clock) {
            return SWD_Read_SPI_Fast (header, data);
        } else {
            return SWD_Read_SPI (header, data);
        }
    } else {
        if (DAP_Data.fast_clock) {
            return SWD_Read_GPIO_Fast (header, DAP_Data.swd_conf.turnaround, DAP_Data.swd_conf.data_phase, DAP_Data.transfer.idle_cycles, data);

        } else {
            return SWD_Read_GPIO (header, data);
        }
    }
}

/**
 * @brief SWD 写操作
 * @param header SWD 头
 * @param data 写数据指针
 * @return ACK 值
 */
uint8_t SWD_Write (uint8_t header, uint32_t *data) {
    if (DAP_Data.swd_spi_sim) {
        if (DAP_Data.fast_clock) {
            return SWD_Write_SPI_Fast (header, data);
        } else {
            return SWD_Write_SPI (header, data);
        }
    } else {
        if (DAP_Data.fast_clock) {
            return SWD_Write_GPIO_Fast (header, DAP_Data.swd_conf.turnaround, DAP_Data.swd_conf.data_phase, DAP_Data.transfer.idle_cycles, data);
        } else {
            return SWD_Write_GPIO (header, data);
        }
    }
}
