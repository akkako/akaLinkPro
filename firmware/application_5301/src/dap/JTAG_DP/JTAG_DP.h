#ifndef JTAG_DP_H
#define JTAG_DP_H

#include <stdint.h>

void JTAG_Sequence_Slow(uint32_t delay, uint32_t info, const uint8_t *tdi, uint8_t *tdo);
uint32_t JTAG_ReadIDCode_Slow(void);
void JTAG_IR_Slow(uint32_t ir);
void JTAG_WriteAbort_Slow(uint32_t data, uint32_t bypass_before, uint32_t bypass_after);
uint8_t JTAG_Write_Slow(uint32_t request, uint32_t *data, uint32_t bypass_before, uint32_t bypass_after);
uint8_t JTAG_Read_Slow(uint32_t request, uint32_t *data, uint32_t bypass_before, uint32_t bypass_after);


uint8_t JTAG_Write_GPIO_ASM_45M(uint32_t request, uint32_t *data, uint32_t bypass_before, uint32_t bypass_after);

#endif
