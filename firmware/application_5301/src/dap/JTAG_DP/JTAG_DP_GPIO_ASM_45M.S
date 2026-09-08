    
.equ FGPIO_BASE,                (0x000C0000)
.equ FGPIO_DI_VAL_OFFSET,       (0x000)
.equ FGPIO_DO_VAL_OFFSET,       (0x100)
.equ FGPIO_DO_SET_OFFSET,       (0x104)
.equ FGPIO_DO_CLR_OFFSET,       (0x108)

/*
    JTCK        PA27
    JTMS        PA28
    JTDI        PA08
    JTDO        PA09
*/

.equ JTCK_SHIFT,                (27)
.equ JTMS_SHIFT,                (28)
.equ JTDI_SHIFT,                (8)
.equ JTDO_SHIFT,                (9)

.equ JTCK_OFFSET,               (1 << JTCK_SHIFT)
.equ JTMS_OFFSET,               (1 << JTMS_SHIFT)
.equ JTDI_OFFSET,               (1 << JTDI_SHIFT)
.equ JTDO_OFFSET,               (1 << JTDO_SHIFT)

.equ TRANFER_OK,                (1)


.section .fast, "ax"
.align  2
.global JTAG_Write_GPIO_ASM_45M
.type   JTAG_Write_GPIO_ASM_45M, @function

// uint8_t JTAG_Write_GPIO_ASM_45M(uint32_t request, uint32_t *data, uint32_t bypass_before, uint32_t bypass_after)

# Used generic register:
# a0 -- uint32_t        request         // [param] info
# a1 -- uint32_t*       data            // [param] data
# a2 -- uint32_t        bypass_before   // [param] bypass before count
# a3 -- uint32_t        bypass_after    // [param] bypass after count
# 
# a4 -- temp
# a5 -- temp
# a6 -- temp
# a7 -- temp
#
# t0 -- FGPIO_BASE                      // [prest]
# t1 -- JTCK_OFFSET                     // [prest]
# t2 -- JTMS_OFFSET                     // [prest]
# t3 -- temp
# t4 -- 
# t5 -- 
# t6 -- 


JTAG_Write_GPIO_ASM_30M:
    li      t0, FGPIO_BASE

    /* Select-DR-Scan */
    lw      t4, FGPIO_DO_VAL_OFFSET(t0)
    bseti   t4, t4, JTMS_SHIFT
    bclri   t4, t4, JTCK_SHIFT
    
    sw      t4, FGPIO_DO_VAL_OFFSET(t0) # ----- TCK 拉低，TMS 拉高，其他不变
    li      t1, JTCK_OFFSET
    li      t2, JTMS_OFFSET
    li      t3, JTDI_OFFSET

    sw      t1, FGPIO_DO_SET_OFFSET(t0) # ----- TCK 拉高 
    lw      t4, FGPIO_DO_VAL_OFFSET(t0)
    bclri   t4, t4, JTMS_SHIFT
    bclri   t4, t4, JTCK_SHIFT

    /* Capture-DR */
    
    sw      t4, FGPIO_DO_VAL_OFFSET(t0) # ----- TCK 拉低，TMS 拉低，其他不变
    nop
    nop
    nop

    sw      t1, FGPIO_DO_SET_OFFSET(t0) # ----- TCK 拉高 
    nop
    nop
    nop


    /* Shift-DR */
    sw      t1, FGPIO_DO_CLR_OFFSET(t0) # ----- TCK 拉低
    nop
    nop
    nop
 
    sw      t1, FGPIO_DO_SET_OFFSET(t0) # ----- TCK 拉高 
    nop
    nop
    nop

    /* Bypass before data (fix zero) */


    /* Send request and receive ack */
    srli    a0, a0, 1                   # a0 >>= 1
    andi    a4, a0, 1                   # a4 = request LSB
    slli    a4, a4, JTDI_SHIFT          # a4 数据偏移到 TDI 数据位(TMS 0 TCK 0)
    sw      a4, FGPIO_DO_VAL_OFFSET(t0) # ----- TCK 拉低, TMS 0, TDI 数据
.rept 16
    nop
.endr
    sw      t1, FGPIO_DO_SET_OFFSET(t0) # ----- TCK 拉高 
    nop
    nop
    nop
    nop
    nop
    lw      a5, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a5 (BIT1) 
    nop
    bexti   a6, a5, JTDO_SHIFT          # 将 TDO 数据移到 LSB，并清除其他位赋值给 a6

    srli    a0, a0, 1                   # a0 >>= 1
    andi    a4, a0, 1                   # a4 = request LSB
    slli    a4, a4, JTDI_SHIFT          # a4 数据偏移到 TDI 数据位(TMS 0 TCK 0)
    sw      a4, FGPIO_DO_VAL_OFFSET(t0) # ----- TCK 拉低, TMS 0, TDI 数据
.rept 16
    nop
.endr
    sw      t1, FGPIO_DO_SET_OFFSET(t0) # ----- TCK 拉高 
    nop
    nop
    nop
    nop
    nop
    lw      a5, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a5 (BIT0) 
    nop
    bexti   a7, a5, JTDO_SHIFT          # 将 TDO 数据移到 LSB，并清除其他位赋值给 a7
    sh1add  a7, a6, a7                  # a7 = (a6 << 1) + a7

    srli    a0, a0, 1                   # a0 >>= 1
    andi    a4, a0, 1                   # a4 = request LSB
    slli    a4, a4, JTDI_SHIFT          # a4 数据偏移到 TDI 数据位(TMS 0 TCK 0)
    sw      a4, FGPIO_DO_VAL_OFFSET(t0) # ----- TCK 拉低, TMS 0, TDI 数据
.rept 16
    nop
.endr
    sw      t1, FGPIO_DO_SET_OFFSET(t0) # ----- TCK 拉高 
    nop
    nop
    nop
    nop
    lw      a5, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a5 (BIT2) 
    nop

    bexti   a6, a5, JTDO_SHIFT          # 将 TDO 数据移到 LSB，并清除其他位赋值给 a6
    sh2add  a7, a6, a7                  # a7 = (a6 << 2) + a7

    mv      a0, a7                      # 返回值 ACK

    li      t4, TRANFER_OK
    bne     a0, t4, .label_transfer_error

    lw      a4, 0(a1)

    /* Write Transfer */
.rept 31
    andi    a5, a4, 1                   # a5 = data LSB
    slli    a5, a5, JTDI_SHIFT          # a5 = 数据偏移到 JTDI 数据位
    
    sw      a5, FGPIO_DO_VAL_OFFSET(t0) # ----- TCK 拉低, TMS 拉低, TDI 数据
    nop
    nop
    nop

    sw      t1, FGPIO_DO_SET_OFFSET(t0) # ----- TCK 拉高 
    srli    a4, a4, 1                   # a4 >>= 1
.endr

    /* Set D31 */
    andi    a5, a4, 1                   # a5 = data LSB
    slli    a5, a5, JTDI_SHIFT          # a5 = 数据偏移到 JTDI 数据位
    
    sw      a5, FGPIO_DO_VAL_OFFSET(t0) # ----- TCK 拉低, TMS 拉低, TDI 数据
    nop
    nop
    nop

    sw      t1, FGPIO_DO_SET_OFFSET(t0) # ----- TCK 拉高 
    nop
    nop
    nop

    /* Bypass & Exit1-DR */
    sw      t2, FGPIO_DO_SET_OFFSET(t0) # TMS 拉高
    sw      t1, FGPIO_DO_CLR_OFFSET(t0) # ----- TCK 拉低
    nop
    nop

    sw      t1, FGPIO_DO_SET_OFFSET(t0) # ----- TCK 拉高 
    nop
    nop
    nop

    /* Update-DR */
    sw      t1, FGPIO_DO_CLR_OFFSET(t0) # ----- TCK 拉低
    nop
    nop
    nop

    sw      t1, FGPIO_DO_SET_OFFSET(t0) # ----- TCK 拉高 
    nop
    nop
    nop

    /* Idle */
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # TMS 拉低
    sw      t1, FGPIO_DO_CLR_OFFSET(t0) # ----- TCK 拉低
    nop
    nop

    sw      t1, FGPIO_DO_SET_OFFSET(t0) # ----- TCK 拉高 
    nop
    nop
    nop

    sw      t3, FGPIO_DO_SET_OFFSET(t0) # TDI 拉高
    ret

.label_transfer_error:
    /* Exit1-DR */
    sw      t2, FGPIO_DO_SET_OFFSET(t0) # TMS 拉高
    sw      t1, FGPIO_DO_CLR_OFFSET(t0) # ----- TCK 拉低
    nop
    nop

    sw      t1, FGPIO_DO_SET_OFFSET(t0) # ----- TCK 拉高 
    nop
    nop
    nop

    /* Update-DR */
    sw      t1, FGPIO_DO_CLR_OFFSET(t0) # ----- TCK 拉低
    nop
    nop
    nop

    sw      t1, FGPIO_DO_SET_OFFSET(t0) # ----- TCK 拉高 
    lw      t4, FGPIO_DO_VAL_OFFSET(t0)
    bclri   t4, t4, JTMS_SHIFT
    bclri   t4, t4, JTCK_SHIFT

    /* Idle */
    sw      t4, FGPIO_DO_VAL_OFFSET(t0) # ----- TCK 拉低，TMS 拉低，其他不变
    nop
    nop
    nop

    sw      t1, FGPIO_DO_SET_OFFSET(t0) # ----- TCK 拉高 
    nop
    nop
    nop

    sw      t3, FGPIO_DO_SET_OFFSET(t0) # TDI 拉高

    ret
