/*
    JTAG_Sequence GPIO 汇编实现（位置无关，可后续动态加载）

    引脚分配（JTAG 模式，PORT_JTAG_SETUP 已把 PA08/09/27/28 挂到 FGPIO）：
        JTCK        PA27   (bit 27)
        JTMS        PA28   (bit 28)
        JTDI        PA08   (bit 8)
        JTDO        PA09   (bit 9, 输入)

    info 位定义（与 C 实现 JTAG_Sequence_Slow 一致）：
        info[5:0]   TCK 个数（0 表示 64）
        info[6]     TMS 值（整段序列保持不变）
        info[7]     TDO 捕获标志

    与 SWD 汇编相同的优化思路：
      - 单条 store 同时建立 TCK 与 TDI 电平（DO_VAL 整字写）
      - TCK 拉高用 DO_SET，不动 TDI/TMS
      - TDI 位运算直接生成寄存器值，无分支
      - TDO 采样在 TCK 高电平末端固定 nop 处，配合电平转换延迟
      - 采样做 1 位流水：本位的 TDO 在下一拍的拉低阶段处理，
        使采样点尽量贴近下一个下降沿，从而缩短 TCK 周期
      - 热路径按字节展开，循环用向后跳转（BTFN 预测 Taken）
*/

.equ FGPIO_BASE,                (0x000C0000)
.equ FGPIO_DI_VAL_OFFSET,       (0x000)
.equ FGPIO_DO_VAL_OFFSET,       (0x100)
.equ FGPIO_DO_SET_OFFSET,       (0x104)
.equ FGPIO_DO_CLR_OFFSET,       (0x108)

.equ JTCK_SHIFT,                (27)
.equ JTMS_SHIFT,                (28)
.equ JTDI_SHIFT,                (8)
.equ JTDO_SHIFT,                (9)

.equ JTCK_OFFSET,               (1 << JTCK_SHIFT)
.equ JTMS_OFFSET,               (1 << JTMS_SHIFT)
.equ JTDI_OFFSET,               (1 << JTDI_SHIFT)
.equ JTDO_OFFSET,               (1 << JTDO_SHIFT)

# ---- 时序标定（与外围电平转换延迟相关，需按实测调整）----
# 捕获路径：拉低阶段 nop / 拉高阶段 nop（决定 TDO 采样点，需覆盖往返延迟）
.equ JTAG_CAP_LOW_NOP,          (4)
.equ JTAG_CAP_HIGH_NOP,         (10)
# 只写路径：无需采样 TDO，可更短
.equ JTAG_WR_LOW_NOP,           (4)
.equ JTAG_WR_HIGH_NOP,          (10)
# 单 bit TMS 导航快速路径半周期 nop
.equ JTAG_1BIT_NOP,             (2)

# 捕获位（1 位流水）：
#   拉低 -> 处理上一拍采样 -> 计算下一位 TDI -> 拉高 -> 末尾采样 TDO
.macro JTAG_BIT_CAPTURE
    sw      t4, FGPIO_DO_VAL_OFFSET(t0)     # TCK 拉低，TDI/TMS 预设
    bexti   a7, a6, JTDO_SHIFT              # 处理上一拍 TDO
    or      a5, a5, a7
    rori    a5, a5, 1
    srli    a4, a4, 1                       # 准备下一位
    andi    t4, a4, JTDI_OFFSET
    or      t4, t4, t2                      # 叠加 TMS

    sw      t1, FGPIO_DO_SET_OFFSET(t0)     # TCK 拉高
.rept JTAG_CAP_HIGH_NOP
    nop
.endr
    lw      a6, FGPIO_DI_VAL_OFFSET(t0)     # 高位末端采样 TDO
.endm

# 只写位（不采样）
.macro JTAG_BIT_WRITE
    sw      t4, FGPIO_DO_VAL_OFFSET(t0)     # TCK 拉低，TDI/TMS 预设
.rept JTAG_WR_LOW_NOP
    nop
.endr
    srli    a4, a4, 1
    andi    t4, a4, JTDI_OFFSET
    or      t4, t4, t2

    sw      t1, FGPIO_DO_SET_OFFSET(t0)     # TCK 拉高
.rept JTAG_WR_HIGH_NOP
    nop
.endr
.endm


.section .fast, "ax"
.align  2
.global JTAG_Sequence_GPIO_ASM_45M
.type   JTAG_Sequence_GPIO_ASM_45M, @function

// void JTAG_Sequence_GPIO_ASM_45M(uint32_t info, const uint8_t *tdi, uint8_t *tdo, uint32_t delay)

JTAG_Sequence_GPIO_ASM_45M:
    li      t0, FGPIO_BASE
    li      t1, JTCK_OFFSET

    # 预计算 TMS 掩码 t2 = (info[6] ? JTMS_OFFSET : 0)
    andi    t2, a0, 0x40
    slli    t2, t2, (JTMS_SHIFT - 6)

    # 快速路径：n==1 且无 TDO（info==0x01/0x41），占调用量 ~85%
    andi    t3, a0, 0xBF
    li      t4, 1
    beq     t3, t4, .L_jtag_1bit

    # n = info[5:0]，0 视为 64
    andi    a3, a0, 0x3F
    bnez    a3, .L_jtag_n_ok
    li      a3, 64
.L_jtag_n_ok:
    # t5 = 完整字节数 = n/8，t6 = 尾部位数 = n%8
    srli    t5, a3, 3
    andi    t6, a3, 7

    # TDO 捕获标志
    andi    t3, a0, 0x80
    beqz    t3, .L_jtag_nocap

    # ================= TDO 捕获路径 =================
    beqz    t5, .L_jtag_cap_tail
.L_jtag_cap_byte:
    lbu     a4, 0(a1)                       # 取 TDI 字节
    addi    a1, a1, 1
    slli    a4, a4, JTDI_SHIFT              # 字节对齐到 TDI 位，逐位右移
    mv      a5, x0                          # 清空 TDO 累加器
    mv      a6, x0                          # 流水首拍空采样
    andi    t4, a4, JTDI_OFFSET             # 预置 bit0 的 DO_VAL
    or      t4, t4, t2
.rept 8
    JTAG_BIT_CAPTURE
.endr
    bexti   a7, a6, JTDO_SHIFT              # 冲刷最后一位采样
    or      a5, a5, a7
    rori    a5, a5, 1
    srli    a4, a5, 24                      # 8 位收集在 [31:24]，右移到低字节
    sb      a4, 0(a2)
    addi    a2, a2, 1
    addi    t5, t5, -1
    bnez    t5, .L_jtag_cap_byte

.L_jtag_cap_tail:
    beqz    t6, .L_jtag_done
    mv      a3, t6                          # 保存尾部位数，用于最终对齐
    lbu     a4, 0(a1)
    slli    a4, a4, JTDI_SHIFT
    mv      a5, x0
    mv      a6, x0
    andi    t4, a4, JTDI_OFFSET
    or      t4, t4, t2
.L_jtag_cap_tail_loop:
    JTAG_BIT_CAPTURE
    addi    t6, t6, -1
    bnez    t6, .L_jtag_cap_tail_loop
    bexti   a7, a6, JTDO_SHIFT
    or      a5, a5, a7
    rori    a5, a5, 1
    li      t5, 32
    sub     t5, t5, a3                      # shift = 32 - tail
    srl     a4, a5, t5                      # 尾部对齐到 LSB
    sb      a4, 0(a2)

.L_jtag_done:
    ret

    # ================= 只写路径 =================
.L_jtag_nocap:
    beqz    t5, .L_jtag_nocap_tail
.L_jtag_nocap_byte:
    lbu     a4, 0(a1)
    addi    a1, a1, 1
    slli    a4, a4, JTDI_SHIFT
    andi    t4, a4, JTDI_OFFSET
    or      t4, t4, t2
.rept 8
    JTAG_BIT_WRITE
.endr
    addi    t5, t5, -1
    bnez    t5, .L_jtag_nocap_byte

.L_jtag_nocap_tail:
    beqz    t6, .L_jtag_done2
    lbu     a4, 0(a1)
    slli    a4, a4, JTDI_SHIFT
    andi    t4, a4, JTDI_OFFSET
    or      t4, t4, t2
.L_jtag_nocap_tail_loop:
    JTAG_BIT_WRITE
    addi    t6, t6, -1
    bnez    t6, .L_jtag_nocap_tail_loop
.L_jtag_done2:
    ret

    # ================= 单 bit（TMS 导航）快速路径 =================
    # info==0x01（TMS=0，不改 TDI）与 info==0x41（TMS=1，TDI=data[0]）
.L_jtag_1bit:
    li      t3, 0x41
    beq     a0, t3, .L_jtag_1bit_tdi
    # ---- info == 0x01 ----
    li      t4, JTMS_OFFSET
    sw      t4, FGPIO_DO_CLR_OFFSET(t0)     # TMS 拉低
    sw      t1, FGPIO_DO_CLR_OFFSET(t0)     # TCK 拉低
.rept JTAG_1BIT_NOP
    nop
.endr
    sw      t1, FGPIO_DO_SET_OFFSET(t0)     # TCK 拉高
    ret

.L_jtag_1bit_tdi:
    # ---- info == 0x41 ----
    lbu     a4, 0(a1)                       # TDI = data[0]
    andi    a4, a4, 1
    slli    a4, a4, JTDI_SHIFT
    or      a4, a4, t2                      # 叠加 TMS
    sw      a4, FGPIO_DO_VAL_OFFSET(t0)     # TCK 拉低, TMS, TDI
.rept JTAG_1BIT_NOP
    nop
.endr
    sw      t1, FGPIO_DO_SET_OFFSET(t0)     # TCK 拉高
    ret
