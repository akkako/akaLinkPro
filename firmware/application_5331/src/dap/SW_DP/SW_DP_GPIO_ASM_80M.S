    
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
    JTMS_IN     PA28
    JTMS_OUT    PA29
    JTMS_DIR    PA30
*/

.equ SWCLK_SHIFT,               27
.equ SWDI_SHIFT,                28
.equ SWDO_SHIFT,                29
.equ SWDIR_SHIFT,               30

.equ SWCLK_OFFSET,              (1 << SWCLK_SHIFT)
.equ SWDI_OFFSET,               (1 << SWDI_SHIFT)
.equ SWDO_OFFSET,               (1 << SWDO_SHIFT)
.equ SWDIR_OFFSET,              (1 << SWDIR_SHIFT)

.equ ACK_OK_MASK,               1
.equ ACK_WAIT_MASK,             2
.equ ACK_FAULT_MASK,            4
.equ ACK_ERROR_MASK,            8

.section .fast, "ax"
.align  2
.global SWD_Write_GPIO_ASM_80M
.type   SWD_Write_GPIO_ASM_80M, @function

// uint8_t SWD_Write_GPIO_ASM_80M(uint8_t header, uint32_t *data);

SWD_Write_GPIO_ASM_80M:
    # 加载预设值和地址
    csrrci  zero, mstatus, 8
    li      t0, FGPIO_BASE
    li      t1, SWDIR_OFFSET
    li      t2, SWCLK_OFFSET
    # 发送 8 位包头
    # 本阶段使用的寄存器：
    # a0 -- header
    # a1 -- data pointer
    # t0 -- FGPIO_BASE
    # t1 -- SWDIR_OFFSET
    # t2 -- SWCLK_OFFSET
    # t3 -- next DO.VALUE calculate
    # t6 -- data

    andi    t3, a0, 1                   # t3 = header LSB
    #---------------- bit 0 ----------------
    .rept 8
    slli    t3, t3, SWDO_SHIFT          # t3 = 数据偏移到 SWDO 数据位
    or      t3, t3, t1                  # t3 = DO[0].VALUE 预设值

    sw      t3, FGPIO_DO_VAL_OFFSET(t0) # --- SWCLK 拉低，同时输出 SWDIO 数据
    srli    a0, a0, 1                   # a0 >>= 1
    andi    t3, a0, 1                   # t3 = 包头 LSB

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高，同时保持 SWDIO 数据
    .endr
    lw      t6, 0(a1)                   # 读取数据指针指向的 32 位数据
    mv      a0, x0                      # 清空 a0
    
    #=========== calc parity bit ===========
    // 旧算法：
    // srli    a6, a5, 16
    // xor     a5, a5, a6      # 32 -- 16
    // srli    a6, a5, 8
    // xor     a5, a5, a6      # 16 -- 8
    // srli    a6, a5, 4
    // xor     a5, a5, a6      # 8 -- 4
    // srli    a6, a5, 2
    // xor     a5, a5, a6      # 4 -- 2
    // srli    a6, a5, 1
    // xor     a5, a5, a6      # 2 -- 1
    // andi    a5, a5, 1       # clear

    // RVB 加速：
    // cpop    a5, t6
    // andi    a5, a5, 1

    # turnaround 周期
    # 本阶段使用的寄存器：
    # t0 -- FGPIO_BASE
    # t1 -- SWDIR_OFFSET
    # t2 -- SWCLK_OFFSET
    # a5 -- parity bit result
    # a6 -- parity calc temp
    # t6 -- data

    #---------------- bit 0 (trn) ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    sw      t1, FGPIO_DO_CLR_OFFSET(t0) # SWDIR 拉低，SWDIO 切换为输入状态
    nop

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    cpop    a5, t6                      # 直接计算校验值
    andi    a5, a5, 1
    

    # ack 周期
    # 本阶段使用的寄存器：
    # a0 -- ack
    # t0 -- FGPIO_BASE
    # t1 -- SWDIR_OFFSET
    # t2 -- SWCLK_OFFSET
    # a5 -- parity bit result
    # a6 -- ack temp read value (parity calc temp)
    # t6 -- data

    # lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT0)
    # srli    a6, a6, (SWDI_SHIFT-0)      # 将 SWDI 数据移到 BIT0
    # andi    a6, a6, 1                   # 提取 BIT0
    # or      a0, a0, a6                  # 合并 BIT0 到 a0

    # lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT1)
    # srli    a6, a6, (SWDI_SHIFT-1)      # 将 SWDI 数据移到 BIT1
    # andi    a6, a6, 2                   # 提取 BIT1
    # or      a0, a0, a6                  # 合并 BIT1 到 a0

    # lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT2)
    # srli    a6, a6, (SWDI_SHIFT-2)      # 将 SWDI 数据移到 BIT2
    # andi    a6, a6, 4                   # 提取 BIT2
    # or      a0, a0, a6                  # 合并 BIT2 到 a0

    #---------------- bit 0 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    nop
    nop

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    nop
    nop
    
    #---------------- bit 1 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    nop
    nop
    
    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    nop
    nop

    #---------------- bit 2 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    nop
    nop

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    nop
    nop

    # turnaround 周期
    # 本阶段使用的寄存器：
    # t0 -- FGPIO_BASE
    # t1 -- SWDIR_OFFSET
    # t2 -- SWCLK_OFFSET
    # t3 -- 跳转判断预加载（ACK_OK）
    # t4 -- 跳转判断预加载（ACK_WAIT）
    # t5 -- 跳转判断预加载（ACK_FAULT）
    # a5 -- parity bit result
    # a6 -- ack temp read value (ack 阶段延迟采样)
    # t6 -- data

    #---------------- bit 3 (send trn) ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    nop
    nop

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    nop
    nop

    #---------------- bit 4 (virtual) ----------------
    nop
    nop
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT0)

    nop
    bexti   a0, a6, SWDI_SHIFT          # 将 SWDI 数据移到 BIT0，并清除其他位赋值给 a0
    nop

    #---------------- bit 5 (virtual) ----------------
    nop
    nop
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT1)

    nop
    bexti   a6, a6, SWDI_SHIFT          # 将 SWDI 数据移到 BIT0，并清除其他位
    sh1add  a0, a6, a0                  # a0 = (a6 << 1) + a0

    #---------------- bit 6 (virtual) ----------------
    nop
    sw      t1, FGPIO_DO_SET_OFFSET(t0) # SWDIR 拉高，SWDIO 切换为输出状态
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT2)

    li      t3, ACK_OK_MASK             # ACK_OK (预加载比较值)
    bexti   a6, a6, SWDI_SHIFT          # 将 SWDI 数据移到 BIT0，并清除其他位
    sh2add  a0, a6, a0                  # a0 = (a6 << 2) + a0

    // sw      t1, FGPIO_DO_SET_OFFSET(t0) # SWDIR 拉高，SWDIO 切换为输出状态
    // li      t3, ACK_OK_MASK             # ACK_OK (预加载比较值)
    
    # 判断跳转
    # 本阶段使用的寄存器：
    # t0 -- FGPIO_BASE
    # t1 -- SWDIR_OFFSET
    # t2 -- SWCLK_OFFSET
    # t3 -- 跳转判断预加载（ACK_OK）
    # t4 -- 跳转判断预加载（ACK_WAIT）
    # t5 -- 跳转判断预加载（ACK_FAULT）
    # a5 -- parity bit result
    # a6 -- ack temp read value (ack 阶段延迟采样)
    # t6 -- data
    bne     a0, t3, .lable_w_ack_not_ok

.lable_w_ack_ok:

    # 发送 32 位数据
    # 本阶段使用的寄存器：
    # t0 -- FGPIO_BASE
    # t1 -- SWDIR_OFFSET
    # t2 -- SWCLK_OFFSET
    # t3 -- send calc temp
    # a5 -- parity bit result
    # a6 -- ack temp read value (ack 阶段延迟采样)
    # t6 -- data

    #=========== send data bit 0-31 ===========
    andi    t3, t6, 1                   # t3 = data LSB
    #---------------- bit 0 ----------------
    .rept 32
    slli    t3, t3, SWDO_SHIFT          # t3 = 数据偏移到 SWDO 数据位
    or      t3, t3, t1                  # t3 = DO[0].VALUE 预设值

    sw      t3, FGPIO_DO_VAL_OFFSET(t0) # --- SWCLK 拉低，同时输出 SWDIO 数据
    srli    t6, t6, 1                   # a0 >>= 1
    andi    t3, t6, 1                   # t3 = 包头 LSB

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高，同时保持 SWDIO 数据
    .endr

    #=========== send parity bit ===========
    slli    t3, a5, SWDO_SHIFT          # t3 = 数据偏移到 SWDO 数据位
    or      t3, t3, t1                  # t3 = DO[0].VALUE 预设值

    sw      t3, FGPIO_DO_VAL_OFFSET(t0) # --- SWCLK 拉低，同时输出 SWDIO 数据
    li      t4, SWDO_OFFSET
    or      t4, t4, t2

    sw      t4, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK, SWDIO 拉高
    csrrsi  zero, mstatus, 8
    ret

.lable_w_ack_not_ok:
    li      t4, ACK_WAIT_MASK           # ACK_WAIT (预加载比较值)
    li      t5, ACK_FAULT_MASK          # ACK_FAULT (预加载比较值)
    beq     a0, t4, .lable_w_ack_wait
    beq     a0, t5, .lable_w_ack_fault
    j       .lable_w_ack_error

.lable_w_ack_wait:
.lable_w_ack_fault:
.lable_w_ack_error:
    li      t4, SWDO_OFFSET
    sw      t4, FGPIO_DO_SET_OFFSET(t0) # --- SWDIO 拉高
    csrrsi  zero, mstatus, 8
    ret




.section .fast, "ax"
.align  2
.global SWD_Read_GPIO_ASM_80M
.type   SWD_Read_GPIO_ASM_80M, @function

// uint8_t SWD_Read_GPIO_ASM_80M(uint8_t header, uint32_t *data);

SWD_Read_GPIO_ASM_80M:
    # 加载预设值和地址
    csrrci  zero, mstatus, 8            # 4 nop
    li      t0, FGPIO_BASE
    li      t1, SWDIR_OFFSET
    li      t2, SWCLK_OFFSET

    # 发送 8 位包头
    # 本阶段使用的寄存器：
    # a0 -- header
    # a1 -- data pointer
    # t0 -- FGPIO_BASE
    # t1 -- SWDIR_OFFSET
    # t2 -- SWCLK_OFFSET
    # t3 -- next DO.VALUE calculate
    # t6 -- data

    andi    t3, a0, 1                   # t3 = header LSB
    #---------------- bit 0 ----------------
    .rept 8
    slli    t3, t3, SWDO_SHIFT          # t3 = 数据偏移到 SWDO 数据位
    or      t3, t3, t1                  # t3 = DO[0].VALUE 预设值

    sw      t3, FGPIO_DO_VAL_OFFSET(t0) # --- SWCLK 拉低，同时输出 SWDIO 数据
    srli    a0, a0, 1                   # a0 >>= 1
    andi    t3, a0, 1                   # t3 = 包头 LSB

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高，同时保持 SWDIO 数据
    .endr
    nop
    nop
    
    # turnaround 周期
    # 本阶段使用的寄存器：
    # t0 -- FGPIO_BASE
    # t1 -- SWDIR_OFFSET
    # t2 -- SWCLK_OFFSET

    #---------------- bit 0 (trn) ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    sw      t1, FGPIO_DO_CLR_OFFSET(t0) # SWDIR 拉低，SWDIO 切换为输入状态
    nop

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    mv      a5, x0                      # 清空 a5 (用于保存接收数据)
    nop

    # ack 周期
    # 本阶段使用的寄存器：
    # a0 -- ack
    # t0 -- FGPIO_BASE
    # t1 -- SWDIR_OFFSET
    # t2 -- SWCLK_OFFSET
    # a6 -- ack temp read value

    # lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT0)
    # srli    a6, a6, (SWDI_SHIFT-0)      # 将 SWDI 数据移到 BIT0
    # andi    a6, a6, 1                   # 提取 BIT0
    # or      a0, a0, a6                  # 合并 BIT0 到 a0

    # lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT1)
    # srli    a6, a6, (SWDI_SHIFT-1)      # 将 SWDI 数据移到 BIT1
    # andi    a6, a6, 2                   # 提取 BIT1
    # or      a0, a0, a6                  # 合并 BIT1 到 a0

    # lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT2)
    # srli    a6, a6, (SWDI_SHIFT-2)      # 将 SWDI 数据移到 BIT2
    # andi    a6, a6, 4                   # 提取 BIT2
    # or      a0, a0, a6                  # 合并 BIT2 到 a0


    #---------------- bit 0 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    nop
    nop

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    nop
    nop

    #---------------- bit 1 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    nop
    nop
    
    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    nop
    nop

    #---------------- bit 2 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    nop
    nop
    
    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    nop
    nop

    #---------------- bit 3 (virtual) ----------------
    nop
    nop
    nop

    nop
    nop
    nop

    #---------------- bit 4 (virtual) ----------------
    nop
    nop
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT0)

    nop
    bexti   a0, a6, SWDI_SHIFT          # 将 SWDI 数据移到 BIT0，并清除其他位赋值给 a0
    nop

    #---------------- bit 5 (virtual) ----------------
    nop
    nop
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT1)

    nop
    bexti   a6, a6, SWDI_SHIFT          # 将 SWDI 数据移到 BIT0，并清除其他位
    sh1add  a0, a6, a0                  # a0 = (a6 << 1) + a0

    #---------------- bit 6 (virtual) ----------------
    nop
    nop
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT2)

    li      t3, ACK_OK_MASK             # ACK_OK (预加载比较值)
    bexti   a6, a6, SWDI_SHIFT          # 将 SWDI 数据移到 BIT0，并清除其他位
    sh2add  a0, a6, a0                  # a0 = (a6 << 2) + a0

    #=========== check ack and branch ===========
    bne     a0, t3, .label_r_not_ack_ok # 如果不是 ACK_OK，跳转到 label_r_not_ack_ok
    
.lable_r_ack_ok:
    # 接收 32 位数据
    # 本阶段使用的寄存器：
    # a0 -- ack
    # t0 -- FGPIO_BASE
    # t1 -- SWDIR_OFFSET
    # t2 -- SWCLK_OFFSET
    # t4 -- recv parity bit
    # t5 -- calc parity bit
    # t6 -- 0x80000000
    # a5 -- data shift
    # a6 -- read DI.VALUE

    // 原本读取一位
    // lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT0)
    // slli    a6, a6, (31-SWDI_SHIFT)     # 将 SWDI 数据移到 BIT31
    // and     a6, a6, t6                  # 提取 BIT31
    // or      a5, a5, a6                  # 合并 BIT31 到 a5
    // srli    a5, a5, 1                   # a5 >>= 1

    // RVB 加速读取一位
    // lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT0)
    // bexti   t6, a6, SWDI_SHIFT          # 将 SWDI 数据移到 BIT0 并清除其他位
    // or      a5, a5, t6                  # 合并 BIT0 到 a5
    // rori    a5, a5, 1                   # a5 循环右移 1 位

    // 注意最后一次直接操作最高位

    #---------------- bit 0 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    mv      t4, x0                      # 清空 t4 (用于保存接收 parity bit)
    mv      a5, x0                      # 清空 a5 (用于保存data)

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    li      t3, 0x80000000
    nop

    #---------------- bit 1 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    nop
    nop

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    nop
    nop

    #---------------- bit 2 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    nop
    nop

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    nop
    nop

    #---------------- bit 3 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    nop
    nop

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    nop
    nop

    #---------------- bit 4 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    nop
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT0)

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    bexti   t6, a6, SWDI_SHIFT          # 将 SWDI 数据移到 BIT0 并清除其他位
    or      a5, a5, t6                  # 合并 BIT0 到 a5

    #---------------- bit 5-31 ----------------
.rept 27
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    rori    a5, a5, 1                   # a5 循环右移 1 位
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT1-27)

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    bexti   t6, a6, SWDI_SHIFT          # 将 SWDI 数据移到 BIT0 并清除其他位
    or      a5, a5, t6                  # 合并 BIT0 到 a5
.endr
    #---------------- bit 32 (recv data 28 / send parity clk) ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    rori    a5, a5, 1                   # a5 循环右移 1 位
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT28)

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    bexti   t6, a6, SWDI_SHIFT          # 将 SWDI 数据移到 BIT0 并清除其他位
    or      a5, a5, t6                  # 合并 BIT0 到 a5

    #---------------- bit 33 (recv data 29 / send trn clk) ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    rori    a5, a5, 1                   # a5 循环右移 1 位
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT29)

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    bexti   t6, a6, SWDI_SHIFT          # 将 SWDI 数据移到 BIT0 并清除其他位
    or      a5, a5, t6                  # 合并 BIT0 到 a5

    #---------------- bit 34 (recv data 30 / no send) ----------------
    sw      t3, FGPIO_DO_SET_OFFSET(t0) # --- SWDIO 拉高
    rori    a5, a5, 1                   # a5 循环右移 1 位
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT30)

    sw      t1, FGPIO_DO_SET_OFFSET(t0) # SWDIR 拉高，SWDIO 切换为输出状态
    bexti   t6, a6, SWDI_SHIFT          # 将 SWDI 数据移到 BIT0 并清除其他位
    or      a5, a5, t6                  # 合并 BIT0 到 a5

    #---------------- bit 35 (recv data 31 / no send) ----------------
    rori    a5, a5, 1                   # a5 循环右移 1 位
    srli    a5, a5, 1                   # a5 >>= 1
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT31)

    slli    a6, a6, (31-SWDI_SHIFT)     # 将 SWDI 数据移到 BIT31
    and     a6, a6, t3                  # 提取 BIT31
    or      a5, a5, a6                  # 合并 BIT31 到 a5

    #---------------- bit 36 (recv parity / no send) ----------------
    li      t3, SWDO_OFFSET
    cpop    t5, a5                      # 直接计算校验位，保存在 t5 BIT0
    lw      t4, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 t4 (parity)
    
    sw      a5, 0(a1)                   # 将接收的数据保存（不管校验是否正确）
    bexti   t4, t4, SWDI_SHIFT          # 将 SWDI 数据移到 BIT0 并清除其他位
    andi    t5, t5, 1
    
    beq     t4, t5, .L_r_parity_check_ok
    li      a0, ACK_ERROR_MASK

.L_r_parity_check_ok:
    csrrsi  zero, mstatus, 8
    ret

.label_r_not_ack_ok:
    # 处理 ACK_WAIT 和 ACK_FAULT
    li      t4, ACK_WAIT_MASK           # ACK_WAIT (预加载比较值)
    li      t5, ACK_FAULT_MASK          # ACK_FAULT (预加载比较值)
    beq     a0, t4, .lable_r_ack_wait   # ACK_WAIT
    beq     a0, t5, .lable_r_ack_fault  # ACK_FAULT
    j       .lable_r_ack_error          # ACK_ERROR

.lable_r_ack_wait:
.lable_r_ack_fault:
.lable_r_ack_error:
    # turnaround 周期
    # 本阶段使用的寄存器：
    # t0 -- FGPIO_BASE
    # t1 -- SWDIR_OFFSET
    # t2 -- SWCLK_OFFSET

    #---------------- bit 0 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    sw      t1, FGPIO_DO_SET_OFFSET(t0) # SWDIR 拉高，SWDIO 切换为输出状态
    li      t3, SWDO_OFFSET

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    sw      t3, FGPIO_DO_SET_OFFSET(t0) # --- SWDIO 拉高
    csrrsi  zero, mstatus, 8
    ret
