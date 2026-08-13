    
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

.equ SWCLK_OFFSET,              0x08000000
.equ SWDI_OFFSET,               0x10000000
.equ SWDO_OFFSET,               0x20000000
.equ SWDIR_OFFSET,              0x40000000

.equ ACK_OK_MASK,               1
.equ ACK_WAIT_MASK,             2
.equ ACK_FAULT_MASK,            4
.equ ACK_ERROR_MASK,            8

.section .fast, "ax"
.align  2
.global SWD_Write_GPIO_ASM_36M
.type   SWD_Write_GPIO_ASM_36M, @function

// uint8_t SWD_Write_GPIO_ASM_36M(uint8_t header, uint32_t *data);

SWD_Write_GPIO_ASM_36M:
    # 加载预设值和地址
    li      t0, FGPIO_BASE
    li      t1, SWDIR_OFFSET
    li      t2, SWCLK_OFFSET

    csrrci  zero, mstatus, 8

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
    nop
    nop
    srli    a0, a0, 1                   # a0 >>= 1
    andi    t3, a0, 1                   # t3 = 包头 LSB
    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高，同时保持 SWDIO 数据
    nop
    nop
    .endr
    lw      t6, 0(a1)                  # 读取数据指针指向的 32 位数据
    mv      a5, t6
    
    
    #=========== calc parity bit ===========
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

    # turnaround 周期
    # 本阶段使用的寄存器：
    # t0 -- FGPIO_BASE
    # t1 -- SWDIR_OFFSET
    # t2 -- SWCLK_OFFSET
    # a5 -- parity bit result
    # a6 -- parity calc temp
    # t6 -- data

    #---------------- bit 0 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    sw      t1, FGPIO_DO_CLR_OFFSET(t0) # SWDIR 拉低，SWDIO 切换为输入状态
    srli    a6, a5, 16
    xor     a5, a5, a6      # 32 -- 16
    srli    a6, a5, 8
    xor     a5, a5, a6      # 16 -- 8


    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    srli    a6, a5, 4
    xor     a5, a5, a6      # 8 -- 4
    srli    a6, a5, 2

    mv      a0, x0                      # 清空 a0

    # ack 周期
    # 本阶段使用的寄存器：
    # a0 -- ack
    # t0 -- FGPIO_BASE
    # t1 -- SWDIR_OFFSET
    # t2 -- SWCLK_OFFSET
    # a5 -- parity bit result
    # a6 -- ack temp read value (parity calc temp)
    # t6 -- data

    #---------------- bit 0 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    xor     a5, a5, a6      # 4 -- 2
    srli    a6, a5, 1
    xor     a5, a5, a6      # 2 -- 1
    andi    a5, a5, 1       # clear

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    nop
    nop
    nop
    nop
    
    #---------------- bit 1 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    nop
    nop
    nop
    nop
    
    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    nop
    nop
    nop
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT0)
    #---------------- bit 2 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    nop
    srli    a6, a6, (SWDI_SHIFT-0)      # 将 SWDI 数据移到 BIT0
    andi    a6, a6, 1                   # 提取 BIT0
    or      a0, a0, a6                  # 合并 BIT0 到 a0

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    nop
    nop
    nop
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT1)
    #---------------- bit 3 (virtual) ----------------
    nop // sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    nop
    srli    a6, a6, (SWDI_SHIFT-1)      # 将 SWDI 数据移到 BIT1
    andi    a6, a6, 2                   # 提取 BIT1
    or      a0, a0, a6                  # 合并 BIT1 到 a0

    nop // sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    nop
    nop
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

    #---------------- bit 0 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    li      t3, ACK_OK_MASK             # ACK_OK (预加载比较值)
    li      t4, ACK_WAIT_MASK           # ACK_WAIT (预加载比较值)
    li      t5, ACK_FAULT_MASK          # ACK_FAULT (预加载比较值)
    sw      t1, FGPIO_DO_SET_OFFSET(t0) # SWDIR 拉高，SWDIO 切换为输出状态

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT2)
    srli    a6, a6, (SWDI_SHIFT-2)      # 将 SWDI 数据移到 BIT2
    andi    a6, a6, 4                   # 提取 BIT2
    or      a0, a0, a6                  # 合并 BIT2 到 a0

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
    beq     a0, t3, .lable_w_ack_ok
    beq     a0, t4, .lable_w_ack_wait
    beq     a0, t5, .lable_w_ack_fault
    j       .lable_w_ack_error

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
    nop
    nop
    srli    t6, t6, 1                   # a0 >>= 1
    andi    t3, t6, 1                   # t3 = 包头 LSB
    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高，同时保持 SWDIO 数据
    nop
    nop
    .endr

    #=========== send parity bit ===========
    slli    t3, a5, SWDO_SHIFT          # t3 = 数据偏移到 SWDO 数据位
    or      t3, t3, t1                  # t3 = DO[0].VALUE 预设值
    sw      t3, FGPIO_DO_VAL_OFFSET(t0) # --- SWCLK 拉低，同时输出 SWDIO 数据
    nop
    nop
    nop
    nop
    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高，同时保持 SWDIO 数据
    nop
    nop
    csrrsi  zero, mstatus, 8

    #=========== SWDIO high ===========
    li      t3, SWDO_OFFSET
    sw      t3, FGPIO_DO_SET_OFFSET(t0) # --- SWDIO 拉高
    ret

.lable_w_ack_wait:
.lable_w_ack_fault:
.lable_w_ack_error:
    csrrsi  zero, mstatus, 8
    ret





.section .fast, "ax"
.align  2
.global SWD_Read_GPIO_ASM_36M
.type   SWD_Read_GPIO_ASM_36M, @function

// uint8_t SWD_Read_GPIO_ASM_36M(uint8_t header, uint32_t *data);

SWD_Read_GPIO_ASM_36M:
    # 加载预设值和地址
    li      t0, FGPIO_BASE
    li      t1, SWDIR_OFFSET
    li      t2, SWCLK_OFFSET

    csrrci  zero, mstatus, 8

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
    #---------------- bit 0 ~ 7 ----------------
    .rept 8
    slli    t3, t3, SWDO_SHIFT          # t3 = 数据偏移到 SWDO 数据位
    or      t3, t3, t1                  # t3 = DO[0].VALUE 预设值
    sw      t3, FGPIO_DO_VAL_OFFSET(t0) # --- SWCLK 拉低，同时输出 SWDIO 数据
    nop
    nop
    srli    a0, a0, 1                   # a0 >>= 1
    andi    t3, a0, 1                   # t3 = 包头 LSB
    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高，同时保持 SWDIO 数据
    nop
    nop
    .endr
    nop
    nop
    
    # turnaround 周期
    # 本阶段使用的寄存器：
    # t0 -- FGPIO_BASE
    # t1 -- SWDIR_OFFSET
    # t2 -- SWCLK_OFFSET

    #---------------- bit 0 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    sw      t1, FGPIO_DO_CLR_OFFSET(t0) # SWDIR 拉低，SWDIO 切换为输入状态
    nop
    nop
    nop


    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    nop
    nop
    mv      a5, x0                      # 清空 a5 (用于保存接收数据)
    mv      a0, x0                      # 清空 a0 (用于保存 ACK)

    # ack 周期
    # 本阶段使用的寄存器：
    # a0 -- ack
    # t0 -- FGPIO_BASE
    # t1 -- SWDIR_OFFSET
    # t2 -- SWCLK_OFFSET
    # a6 -- ack temp read value

    #---------------- bit 0 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    nop
    nop
    nop
    nop

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    nop
    nop
    nop
    nop
    
    #---------------- bit 1 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    nop
    nop
    nop
    nop
    
    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    nop
    nop
    nop
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT0)
    #---------------- bit 2 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    nop
    srli    a6, a6, (SWDI_SHIFT-0)      # 将 SWDI 数据移到 BIT0
    andi    a6, a6, 1                   # 提取 BIT0
    or      a0, a0, a6                  # 合并 BIT0 到 a0

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    nop
    nop
    nop
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT1)
    #---------------- bit 3 (virtual) ----------------
    nop // sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    nop
    srli    a6, a6, (SWDI_SHIFT-1)      # 将 SWDI 数据移到 BIT1
    andi    a6, a6, 2                   # 提取 BIT1
    or      a0, a0, a6                  # 合并 BIT1 到 a0

    // nop // sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    li      t3, ACK_OK_MASK             # ACK_OK (预加载比较值)
    li      t4, ACK_WAIT_MASK           # ACK_WAIT (预加载比较值)
    li      t5, ACK_FAULT_MASK          # ACK_FAULT (预加载比较值)
    li      t6, 0x80000000              # 0x80000000 (预加载筛选值)
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT2)
    
    #---------------- bit 4 (virtual) ----------------
    nop // sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    nop
    srli    a6, a6, (SWDI_SHIFT-2)      # 将 SWDI 数据移到 BIT2
    andi    a6, a6, 4                   # 提取 BIT2
    or      a0, a0, a6                  # 合并 BIT2 到 a0

    // sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    // li      t3, ACK_OK_MASK             # ACK_OK (预加载比较值)
    // li      t4, ACK_WAIT_MASK           # ACK_WAIT (预加载比较值)
    // li      t5, ACK_FAULT_MASK          # ACK_FAULT (预加载比较值)
    // li      t6, 0x80000000              # 0x80000000 (预加载筛选值)

    #=========== check ack and branch ===========
    beq     a0, t3, .lable_r_ack_ok     # ACK_OK
    beq     a0, t4, .lable_r_ack_wait   # ACK_WAIT
    beq     a0, t5, .lable_r_ack_fault  # ACK_FAULT
    j       .lable_r_ack_error          # ACK_ERROR

.lable_r_ack_ok:
    # 接收 32 位数据
    # 本阶段使用的寄存器：
    # a0 -- ack
    # t0 -- FGPIO_BASE
    # t1 -- SWDIR_OFFSET
    # t2 -- SWCLK_OFFSET

    # t6 -- 0x80000000
    # a5 -- data shift
    # a6 -- read DI.VALUE

    // lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT0)
    // slli    a6, a6, (31-SWDI_SHIFT)     # 将 SWDI 数据移到 BIT31
    // and     a6, a6, t6                  # 提取 BIT31
    // or      a5, a5, a6                  # 合并 BIT31 到 a5
    // srli    a5, a5, 1                   # a5 >>= 1
    #---------------- bit 0 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    nop
    nop
    nop
    nop

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    nop
    nop
    nop
    nop
    
    #---------------- bit 1 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    nop
    nop
    nop
    nop

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    nop
    nop
    nop
    nop

    #---------------- bit 2 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT0)
    slli    a6, a6, (31-SWDI_SHIFT)     # 将 SWDI 数据移到 BIT31
    and     a6, a6, t6                  # 提取 BIT31
    or      a5, a5, a6                  # 合并 BIT31 到 a5

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    srli    a5, a5, 1                   # a5 >>= 1
    nop
    nop
    nop
    #---------------- bit 3 ~ 30 ----------------
.rept 28
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT0)
    slli    a6, a6, (31-SWDI_SHIFT)     # 将 SWDI 数据移到 BIT31
    and     a6, a6, t6                  # 提取 BIT31
    or      a5, a5, a6                  # 合并 BIT31 到 a5

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    srli    a5, a5, 1                   # a5 >>= 1
    nop
    nop
    nop
.endr
    #---------------- bit 31 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT29)
    slli    a6, a6, (31-SWDI_SHIFT)     # 将 SWDI 数据移到 BIT31
    and     a6, a6, t6                  # 提取 BIT31
    or      a5, a5, a6                  # 合并 BIT31 到 a5

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    srli    a5, a5, 1                   # a5 >>= 1
    nop
    nop
    nop

    #---------------- bit 32 (virtual/parity clk) ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT30)
    slli    a6, a6, (31-SWDI_SHIFT)     # 将 SWDI 数据移到 BIT31
    and     a6, a6, t6                  # 提取 BIT31
    or      a5, a5, a6                  # 合并 BIT31 到 a5

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    srli    a5, a5, 1                   # a5 >>= 1
    nop
    nop
    nop

    #---------------- bit 33 (virtual) ----------------
    nop //sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    lw      a6, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 a6 (BIT29)
    slli    a6, a6, (31-SWDI_SHIFT)     # 将 SWDI 数据移到 BIT31
    and     a6, a6, t6                  # 提取 BIT31
    or      a5, a5, a6                  # 合并 BIT31 到 a5

    mv      t6, a5 //sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    srli    a6, a5, 16
    xor     a5, a5, a6      # 32 -- 16
    srli    a6, a5, 8
    xor     a5, a5, a6      # 16 -- 8 

    #---------------- bit 34 (virtual/trn) ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    lw      t4, FGPIO_DI_VAL_OFFSET(t0) # 读取 DI.VALUE 到 t4 (parity)
    srli    t4, t4, (SWDI_SHIFT-0)      # 将 SWDI 数据移到 BIT0
    andi    t4, t4, 1                   # 提取 BIT0
    srli    a6, a5, 4 
    
    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    sw      t1, FGPIO_DO_SET_OFFSET(t0) # SWDIR 拉高，SWDIO 切换为输出状态
    xor     a5, a5, a6      # 8 -- 4 
    srli    a6, a5, 2
    xor     a5, a5, a6      # 4 -- 2
    srli    a6, a5, 1
    xor     a5, a5, a6      # 2 -- 1
    andi    a5, a5, 1       # clear other bit

    xor     t4, t4, a5
    andi    t4, t4, 1       # clear other bit
    beq     t4, x0, .L_r_parity_check_ok
    li      a0, ACK_ERROR_MASK

.L_r_parity_check_ok:
    sw      t6, 0(a1)       # save data in memory

    csrrsi  zero, mstatus, 8
    #=========== SWDIO high ===========
    li      t3, SWDO_OFFSET
    sw      t3, FGPIO_DO_SET_OFFSET(t0) # --- SWDIO 拉高
    ret

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
    // andi    a5, a5, 1       # clear other bit

.lable_r_ack_wait:
.lable_r_ack_fault:
    # turnaround 周期
    # 本阶段使用的寄存器：
    # t0 -- FGPIO_BASE
    # t1 -- SWDIR_OFFSET
    # t2 -- SWCLK_OFFSET

    #---------------- bit 0 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    sw      t1, FGPIO_DO_SET_OFFSET(t0) # SWDIR 拉高，SWDIO 切换为输出状态
    nop
    nop
    nop

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    csrrsi  zero, mstatus, 8
    #=========== SWDIO high ===========
    li      t3, SWDO_OFFSET
    sw      t3, FGPIO_DO_SET_OFFSET(t0) # --- SWDIO 拉高
    ret

.lable_r_ack_error:
    # turnaround 周期
    # 本阶段使用的寄存器：
    # t0 -- FGPIO_BASE
    # t1 -- SWDIR_OFFSET
    # t2 -- SWCLK_OFFSET

    #---------------- bit 0 ----------------
    sw      t2, FGPIO_DO_CLR_OFFSET(t0) # --- SWCLK 拉低
    sw      t1, FGPIO_DO_SET_OFFSET(t0) # SWDIR 拉高，SWDIO 切换为输出状态
    nop
    nop
    nop

    sw      t2, FGPIO_DO_SET_OFFSET(t0) # --- SWCLK 拉高
    csrrsi  zero, mstatus, 8
    #=========== SWDIO high ===========
    li      t3, SWDO_OFFSET
    sw      t3, FGPIO_DO_SET_OFFSET(t0) # --- SWDIO 拉高
    ret
