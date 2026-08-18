    
.equ FGPIO_BASE,                0x000C0000
.equ IOA_DI_VAL,                0x000
.equ IOB_DI_VAL,                0x010
.equ IOA_DO_VAL,                0x100
.equ IOA_DO_SET,                0x104
.equ IOA_DO_CLR,                0x108
.equ IOA_DO_TOG,                0x10C
.equ IOB_DO_VAL,                0x110
.equ IOB_DO_SET,                0x114
.equ IOB_DO_CLR,                0x118
.equ IOB_DO_TOG,                0x11C

/*
    JTCK        PA27
    JTMS        PA29
    JTDI        PB13
    JTDO        PB12
*/

.equ JTCK_SHIFT,                27
.equ JTMS_SHIFT,                29
.equ JTDI_SHIFT,                13
.equ JTDO_SHIFT,                12

.equ JTCK_OFFSET,               (1 << JTCK_SHIFT)
.equ JTMS_OFFSET,               (1 << JTMS_SHIFT)
.equ JTDI_OFFSET,               (1 << JTDI_SHIFT)
.equ JTDO_OFFSET,               (1 << JTDO_SHIFT)


.section .fast, "ax"
.align  2
.global JTAG_Sequence_GPIO_ASM_30M
.type   JTAG_Sequence_GPIO_ASM_30M, @function

// void JTAG_Sequence_GPIO_ASM_30M(uint32_t delay, uint32_t info, const uint8_t *tdi, uint8_t *tdo);

JTAG_Sequence_GPIO_ASM_30M:

    li      t1, JTCK_OFFSET

    # Used generic register:
    # a0 -- uint32_t        delay           // [param] JTAG sequence bit delay
    # a1 -- uint32_t        info            // [param] JTAG sequence configure
    # a2 -- const uint8_t*  tdi             // [param] JTAG sequence TDI data pointer
    # a3 -- uint8_t*        tdo             // [param] JTAG sequence TDO data pointer
    # 
    # a4 -- uint32_t        send_bits       // [local] Sequence bit num
    # a5 -- uint8_t         i_val           // [local] 
    # a6 -- uint8_t         o_val           // [local]
    # a7 -- 
    #
    # t0 -- FGPIO_BASE                      // [prest]
    # t1 -- JTCK_OFFSET                     // [prest]
    # t2 -- 
    # t3 -- 
    # t4 -- 
    # t5 -- 
    # t6 -- temp

    bexti   t6, a1, 6                       # 提取 TMS 状态
    slli    t6, t6, JTMS_SHIFT              # 位移到 JTMS_SHIFT 位

    sw      t6, FGPIO_BASE(IOA_DO_VAL)      # ----- JTCK 拉低，同时输出 JTMS
    nop
    nop
    nop

    sw      t1, FGPIO_BASE(IOA_DO_SET)      # ----- JTCK 拉高，同时保持 JTMS
    nop
    nop
    nop

    ret
    

    // Send and read one bit:
    // 


    ret