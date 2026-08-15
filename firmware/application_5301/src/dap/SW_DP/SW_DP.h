#ifndef SW_DP_H
#define SW_DP_H

#include <stdint.h>

void SWJ_Sequence_GPIO_Slow(uint32_t count, const uint8_t *data);
void SWD_Sequence_GPIO_Slow(uint32_t info, const uint8_t *swdo, uint8_t *swdi);
uint8_t SWD_Write_GPIO_Slow(uint8_t header, uint32_t *data);
uint8_t SWD_Read_GPIO_Slow(uint8_t header, uint32_t *data);

uint8_t SWD_Write_GPIO_ASM_30M(uint8_t header, uint32_t *data);
uint8_t SWD_Read_GPIO_ASM_30M(uint8_t header, uint32_t *data);
uint8_t SWD_Write_GPIO_ASM_36M(uint8_t header, uint32_t *data);
uint8_t SWD_Read_GPIO_ASM_36M(uint8_t header, uint32_t *data);
uint8_t SWD_Write_GPIO_ASM_45M(uint8_t header, uint32_t *data);
uint8_t SWD_Read_GPIO_ASM_45M(uint8_t header, uint32_t *data);
#endif