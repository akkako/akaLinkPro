
# SWCLK -- PB13
# SWDO  -- PB15
# SWDI  -- PB14
# SWDIR -- PB12

    .equ GPIOB_BASE,        0x40010C00      # GPIOB 寄存器基地址

    .equ CFGLR_OS,          0x00            # CFGLR 寄存器偏移地址
    .equ CFGHR_OS,          0x04            # CFGHR 寄存器偏移地址
    .equ INDR_OS,           0x08            # INDR 寄存器偏移地址
    .equ OUTDR_OS,          0x0C            # OUTDR 寄存器偏移地址
    .equ BSHR_OS,           0x10            # BSHR 寄存器偏移地址
    .equ BCR_OS,            0x14            # BCR 寄存器偏移地址

    .equ SWCLK_HIGH_MASK,   0x00002000      # SWCLK 输出高电平掩码（BSHR）
    .equ SWCLK_LOW_MASK,    0x20000000      # SWCLK 输出低电平掩码（BSHR）
    .equ SWDIO_HIGH_MASK,   0x00008000      # SWDIO 输出高电平掩码（BSHR）
    .equ SWDIO_LOW_MASK,    0x80000000      # SWDIO 输出低电平掩码（BSHR）

    .equ CFG_INPUT_MASK,    0x44334444      # SWCLK 输出，SWDIO 输入配置（CFGHR）
    .equ CFG_OUTPUT_MASK,   0x34334444      # SWCLK 输出，SWDIO 输出配置（CFGHR）

    .equ DIR_INPUT_MASK,    0x10000000      # SWDIR 输入状态电平掩码（BSHR）
    .equ DIR_OUTPUT_MASK,   0x00001000      # SWDIR 输出状态电平掩码（BSHR）

    .equ ACK_OK_MASK,       0x01            # ACK OK 状态定义
    .equ ACK_WAIT_MASK,     0x02            # ACK WAIT 状态定义
    .equ ACK_FAULT_MASK,    0x04            # ACK FAULT 状态定义
    .equ ACK_ERROR_MASK,    0x08            # ACK ERROR 状态定义

    .text
    .align  2
    .global SWD_Read_GPIO_Fast
    .type   SWD_Read_GPIO_Fast, @function

# definition
# uint8_t SWD_Read_GPIO_Fast(uint8_t header, uint8_t turnaround, uint8_t data_phase, uint8_t idle_cycles, uint32_t *data)
#
# parameter:
# a0 -- uint8_t header (already zero-extended)
# a1 -- uint8_t turnaround (already zero-extended)
# a2 -- uint8_t data_phase (already zero-extended)
# a3 -- uint8_t idle_cycles (already zero-extended)
# a4 -- uint32_t *data
#
# return value:
# a0 -- ack (zero-extend to 32-bit)

# Register allocate
# a0 -- header, ack return data
# a1 -- uint8_t turnaround (already zero-extended)
# a2 -- uint8_t data_phase (already zero-extended)
# a3 -- uint8_t idle_cycles (already zero-extended)
# a4 -- uint32_t *data
# a5 -- Temporary
# a6 -- Temporary
# a7 -- Temporary
# t0 -- GPIOB_BASE address
# t1 -- SWCLK_HIGH_MASK preset
# t2 -- CLK_FALLING_HIGH_MASK preset
# t3 -- CLK_FALLING_LOW_MASK preset
# t4 -- Temporary
# t5 -- Temporary
# t6 -- Temporary

SWD_Read_GPIO_Fast:
    li      t0, GPIOB_BASE
    li      t1, SWCLK_HIGH_MASK
    li      t2, SWCLK_LOW_MASK
    li      t3, SWDIO_LOW_MASK

#=========== send header bits ===========
.rept 8
    andi    t4, a0, 1                       # t4 = a0 & 0x1
    slli    t5, t4, 4                       # t5 = t4 << 4 (t5 = 16 or 0)
    srl     t4, t3, t5                      # t4 = t4 ? BIT15 : BIT31 (calc SWDIO should SET or RST)
    or      t4, t4, t2                      # merge (SWDIO SET/RST and SWCLK RST)
    sw      t4, BSHR_OS(t0)                 # apply calculate SWDIO and SWCLK data
    srli    a0, a0, 1                       # logic right shift for next bit
    sw      t1, BSHR_OS(t0)                 # clock rising and data holding
.endr
#=========== turnaround ===========
    li      t4, CFG_INPUT_MASK              # load config and change SWDIO input
    li      t5, DIR_INPUT_MASK              # load config and set DIR to input
    or      t5, t5, t2
    sw      t4, CFGHR_OS(t0)  
    sw      t5, BSHR_OS(t0)                 # generate clock falling and set DIR to input
    sw      t1, BSHR_OS(t0)                 # generate clock rising and data sampling
#=========== sampling ack ===========
    sw      t2, BSHR_OS(t0)                 # generate clock falling
    lw      t4, INDR_OS(t0)                 # read INDR to t4
    nop
    nop
    nop
    sw      t1, BSHR_OS(t0)                 # generate clock rising
    srli    t4, t4, 14                      # move useful bit to bit 0
    andi    t4, t4, 1                       # clear other bit
    or      a0, a0, t4                      # save to a0

    sw      t2, BSHR_OS(t0)                 # generate clock falling
    lw      t4, INDR_OS(t0)                 # read INDR to t4
    nop
    nop
    nop
    sw      t1, BSHR_OS(t0)                 # generate clock rising
    srli    t4, t4, 13                      # move useful bit to bit 1
    andi    t4, t4, 2                       # clear other bit
    or      a0, a0, t4                      # save to a0

    sw      t2, BSHR_OS(t0)                 # generate clock falling
    lw      t4, INDR_OS(t0)                 # read INDR to t4
    nop
    nop
    nop
    sw      t1, BSHR_OS(t0)                 # generate clock rising
    srli    t4, t4, 12                      # move useful bit to bit 2
    andi    t4, t4, 4                       # clear other bit
    or      a0, a0, t4                      # save to a0

#=========== check ack and branch ===========
    li      t4, ACK_OK_MASK                 # ACK_OK
    beq     a0, t4, .lable_r_ack_ok
    li      t4, ACK_WAIT_MASK               # ACK_WAIT
    beq     a0, t4, .lable_r_ack_wait
    li      t4, ACK_FAULT_MASK              # ACK_FAULT
    beq     a0, t4, .lable_r_ack_fault
    j       .lable_r_ack_error

.lable_r_ack_ok:
#=========== read data bit 0-31 ===========
    li      t4, 0x80000000
    mv      t6, x0
    mv      a7, x0

# read bit 0 in a5
    sw      t2, BSHR_OS(t0)                 # generate clock falling
    lw      a5, INDR_OS(t0)                 # read INDR to a5
    nop
    nop
    nop
    sw      t1, BSHR_OS(t0)                 # generate clock rising

.rept 15
# read bit 1(29) in a6
    sw      t2, BSHR_OS(t0)                 # generate clock falling
    lw      a6, INDR_OS(t0)                 # read INDR to a6
# process bit 0(28) in a5 to t6
    slli    a5, a5, 17                      # move useful bit to bit 31
    and     a5, a5, t4                      # clear other bit
    or      t6, t6, a5                      # save to t6
    srli    t6, t6, 1                       # shift
    sw      t1, BSHR_OS(t0)                 # generate clock rising
    add     a7, a7, a5                      # calc parity
# read bit 2(30) in a5
    sw      t2, BSHR_OS(t0)                 # generate clock falling
    lw      a5, INDR_OS(t0)                 # read INDR to a5
# process bit 1(29) in a6 to t6
    slli    a6, a6, 17                      # move useful bit to bit 31
    and     a6, a6, t4                      # clear other bit
    or      t6, t6, a6                      # save to t6
    srli    t6, t6, 1                       # shift
    sw      t1, BSHR_OS(t0)                 # generate clock rising
    add     a7, a7, a6                      # calc parity

.endr

# read bit 31 in a6
    sw      t2, BSHR_OS(t0)                 # generate clock falling
    lw      a6, INDR_OS(t0)                 # read INDR to a6
# process bit 30 in a5 to t6
    slli    a5, a5, 17                      # move useful bit to bit 31
    and     a5, a5, t4                      # clear other bit
    or      t6, t6, a5                      # save to t6
    srli    t6, t6, 1                       # shift
    sw      t1, BSHR_OS(t0)                 # generate clock rising
    add     a7, a7, a5                      # calc parity

# read parity in a5
    sw      t2, BSHR_OS(t0)                 # generate clock falling
    lw      a5, INDR_OS(t0)                 # read INDR to a5
# process bit 31 in a6 to t6
    slli    a6, a6, 17                      # move useful bit to bit 31
    and     a6, a6, t4                      # clear other bit
    or      t6, t6, a6                      # save to t6
    sw      t1, BSHR_OS(t0)                 # generate clock rising
    add     a7, a7, a6                      # calc parity

# process parity bit in a5 to t4
    srli    a5, a5, 14                      # move useful bit to bit 0
    andi    t4, a5, 1                       # clear other bit
    srli    a7 ,a7, 31

#=========== calc parity bit  ===========
    // mv      a5, t6
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

    xor     t4, t4, a5
    andi    t4, t4, 1       # clear other bit
    beq     t4, x0, .L_r_parity_check_ok
    li      a0, ACK_ERROR_MASK

.L_r_parity_check_ok:
    sw      t6, 0(a4)       # save data in memory

.lable_r_ack_wait:
.lable_r_ack_fault:
#=========== turnaround ===========
    li      t4, CFG_OUTPUT_MASK             # swdio config input
    li      t5, DIR_OUTPUT_MASK             # swdir switch input
    or      t5, t5, t2
    sw      t4, CFGHR_OS(t0)  
    sw      t5, BSHR_OS(t0)                 # generate clock falling and swdir switch input

    sw      t1, BSHR_OS(t0)                 # generate clock rising
    ret

.lable_r_ack_error:
#=========== empty read 33 bit ===========
.rept 33
    sw      t2, BSHR_OS(t0)                 # generate clock falling
    sw      t1, BSHR_OS(t0)                 # generate clock rising
.endr
#=========== turnaround ===========
    li      t5, CFG_OUTPUT_MASK             # swdio config input
    sw      t5, CFGHR_OS(t0)  
    li      t5, DIR_OUTPUT_MASK             # swdir switch input
    sw      t5, BSHR_OS(t0)

    sw      t2, BSHR_OS(t0)                 # generate clock falling
    sw      t1, BSHR_OS(t0)                 # generate clock rising
    ret

#==============================================================================

    .text
    .align  2
    .global SWD_Write_GPIO_Fast
    .type   SWD_Write_GPIO_Fast, @function

# definition
# uint8_t SWD_Write_GPIO_Fast(uint8_t header, uint8_t turnaround, uint8_t data_phase, uint8_t idle_cycles, uint32_t *data)
#
# parameter:
# a0 -- uint8_t header (already zero-extended)
# a1 -- uint8_t turnaround (already zero-extended)
# a2 -- uint8_t data_phase (already zero-extended)
# a3 -- uint8_t idle_cycles (already zero-extended)
# a4 -- uint32_t *data
#
# return value:
# a0 -- ack (zero-extend to 32-bit)

# Register allocate
# a0 -- header, ack return data
# a1 -- uint8_t turnaround (already zero-extended)
# a2 -- uint8_t data_phase (already zero-extended)
# a3 -- uint8_t idle_cycles (already zero-extended)
# a4 -- uint32_t *data
# a5 -- parity calculate temp
# a6 -- parity calculate temp
# a7 -- Unused
# t0 -- R32_GPIOB_BSHR address
# t1 -- SWCLK_HIGH_MASK preset
# t2 -- CLK_FALLING_HIGH_MASK preset
# t3 -- CLK_FALLING_LOW_MASK preset
# t4 -- Temporary
# t5 -- Temporary
# t6 -- Temporary

SWD_Write_GPIO_Fast:
    li      t0, GPIOB_BASE                  # preload GPIOB Base address
    li      t1, SWCLK_HIGH_MASK             # clock rising and data hold preset
    li      t2, SWCLK_LOW_MASK              # clock falling and data high preset
    li      t3, SWDIO_LOW_MASK              # clock falling and data low preset

#=========== send header bits ===========
.rept 8
    andi    t4, a0, 1                       # t4 = a0 & 0x1
    slli    t5, t4, 4                       # t5 = t4 << 4 (t5 = 16 or 0)
    srl     t4, t3, t5                      # t4 = t4 ? BIT15 : BIT31 (calc SWDIO should SET or RST)
    or      t4, t4, t2                      # merge (SWDIO SET/RST and SWCLK RST)
    sw      t4, BSHR_OS(t0)                 # apply calculate SWDIO and SWCLK data
    srli    a0, a0, 1                       # logic right shift for next bit
    sw      t1, BSHR_OS(t0)                 # clock rising and data holding
.endr
#=========== turnaround ===========
    li      t4, CFG_INPUT_MASK              # load config and change SWDIO input
    li      t5, DIR_INPUT_MASK              # load config and set DIR to input
    or      t5, t5, t2
    
    sw      t4, CFGHR_OS(t0)                # and change SWDIO input
    sw      t5, BSHR_OS(t0)                 # generate clock falling and swdir switch input
    sw      t1, BSHR_OS(t0)                 # generate clock rising and data sampling
#=========== sampling ack (mix parity calc) ===========
    // ===== load data =====
    lw      a7, 0(a4)                       # load 32-bit data in a7 and a5
    mv      a5, a7

    sw      t2, BSHR_OS(t0)                 # generate clock falling
    lw      t4, INDR_OS(t0)                 # read INDR to t4
    // ==== parity calc insert ====
    srli    a6, a5, 16
    xor     a5, a5, a6      # 32 -- 16
    srli    a6, a5, 8
    // ==== parity calc insert ====
    sw      t1, BSHR_OS(t0)                 # generate clock rising
    srli    t4, t4, 14                      # move useful bit to bit 0
    andi    t4, t4, 1                       # clear other bit
    or      a0, a0, t4                      # save to a0

    sw      t2, BSHR_OS(t0)                 # generate clock falling
    lw      t4, INDR_OS(t0)                 # read INDR to t4
    // ==== parity calc insert ====
    xor     a5, a5, a6      # 16 -- 8
    srli    a6, a5, 4
    xor     a5, a5, a6      # 8 -- 4
    // ==== parity calc insert ====
    sw      t1, BSHR_OS(t0)                 # generate clock rising
    srli    t4, t4, 13                      # move useful bit to bit 1
    andi    t4, t4, 2                       # clear other bit
    or      a0, a0, t4                      # save to a0

    sw      t2, BSHR_OS(t0)                 # generate clock falling
    lw      t4, INDR_OS(t0)                 # read INDR to t4
    // ==== parity calc insert ====
    srli    a6, a5, 2
    xor     a5, a5, a6      # 4 -- 2
    srli    a6, a5, 1
    // ==== parity calc insert ====
    sw      t1, BSHR_OS(t0)                 # generate clock rising
    srli    t4, t4, 12                      # move useful bit to bit 2
    andi    t4, t4, 4                       # clear other bit
    or      a0, a0, t4                      # save to a0
#=========== turnaround ===========
    sw      t2, BSHR_OS(t0)                 # generate clock falling

    li      t4, CFG_OUTPUT_MASK             # swdio config input
    li      t5, DIR_OUTPUT_MASK             # swdir switch input
    sw      t4, CFGHR_OS(t0)
    // ==== parity calc insert ====
    xor     a5, a5, a6      # 2 -- 1
    // ==== parity calc insert ====
    or      t5, t5, t1
    sw      t5, BSHR_OS(t0)                 # generate clock rising and swdir switch input

#=========== check ack and branch ===========
    li      t6, ACK_OK_MASK                 # ACK_OK
    beq     a0, t6, .lable_w_ack_ok
    li      t6, ACK_WAIT_MASK               # ACK_WAIT
    beq     a0, t6, .lable_w_ack_wait
    li      t6, ACK_FAULT_MASK              # ACK_FAULT
    beq     a0, t6, .lable_w_ack_fault
    j       .lable_w_ack_error

.lable_w_ack_ok:
#=========== load data ===========
    // lw      a7, 0(a4)      # load 32-bit data in a7
    // mv      a5, a7
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
    // parity bit store in a5

#=========== send data bit 0-31 ===========
.rept 32
    andi    t5, a7, 1                       # t5 = a7 & 0x1
    slli    t6, t5, 4                       # t6 = t5 << 4 (t6 = 16 or 0)
    srl     t5, t3, t6                      # t5 = t6 ? BIT15 : BIT31 (calc SWDIO should SET or RST)
    or      t5, t5, t2                      # merge (SWDIO SET/RST and SWCLK RST)
    sw      t5, BSHR_OS(t0)                 # apply calculate SWDIO and SWCLK data
    srli    a7, a7, 1                       # logic right shift for next send
    sw      t1, BSHR_OS(t0)                 # clock rising and data holding
.endr
#=========== send parity bit ===========
    andi    t5, a5, 1                       # save LSB in t5
    slli    t6, t5, 4                       # t6 = t5 ? 16 : 0 
    srl     t5, t3, t6                      # t5 = t6 ? BIT15 : BIT31
    or      t5, t5, t2                      # merge 
    sw      t5, BSHR_OS(t0)
    sw      t1, BSHR_OS(t0)                 # clock rising and data holding

.lable_w_ack_wait:
.lable_w_ack_fault:
.lable_w_ack_error:
    li      t4, SWDIO_HIGH_MASK
    sw      t4, BSHR_OS(t0)                 # output data high

    ret
