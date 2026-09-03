

    
.equ FGPIO_BASE,                0x000C0000
.equ FGPIO_DI_VAL_OFFSET,       0x000
.equ FGPIO_DO_VAL_OFFSET,       0x100
.equ FGPIO_DO_SET_OFFSET,       0x104
.equ FGPIO_DO_CLR_OFFSET,       0x108
.equ FGPIO_OE_VAL_OFFSET,       0x200
.equ FGPIO_OE_SET_OFFSET,       0x204
.equ FGPIO_OE_CLR_OFFSET,       0x208

/*
    JTCK        PA27
    JTMS        PA28
    JTMS_DIR    PA30
 */

.equ SWCLK_SHIFT,               27
.equ SWDIO_SHIFT,               28
.equ SWDIR_SHIFT,               30

.equ SWCLK_OFFSET,              (1 << SWCLK_SHIFT)
.equ SWDIO_OFFSET,              (1 << SWDIO_SHIFT)
.equ SWDIR_OFFSET,              (1 << SWDIR_SHIFT)

.equ ACK_OK_MASK,               1
.equ ACK_WAIT_MASK,             2
.equ ACK_FAULT_MASK,            4
.equ ACK_ERROR_MASK,            8

// SWCLK 下降沿到 SWDIO 输入采样点延迟：15 机器周期
// 适配 SN74LVC1T45DBVR

// ACK 读取流程：
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT0)
    bexti   a0, a6, SWDIO_SHIFT         # 将 SWDI 数据移到 BIT0，并清除其他位赋值给 a0

    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT1)
    bexti   a6, a6, SWDIO_SHIFT         # 将 SWDI 数据移到 BIT0，并清除其他位
    sh1add  a0, a6, a0                  # a0 = (a6 << 1) + a0

    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT2)
    bexti   a6, a6, SWDIO_SHIFT         # 将 SWDI 数据移到 BIT0，并清除其他位
    sh2add  a0, a6, a0                  # a0 = (a6 << 2) + a0