# SWD GPIO ASM 位置无关代码 blob 构建脚本
#
# 用法:
#   make            # 构建默认 VARIANT(60M)的 blob
#   make VARIANT=30M # 构建其他速度版本
#   make clean
#
# 编译选项与 hpm-sdk cmake 保持一致(见 build/compile_commands.json),
# 工具链使用 HPM SDK 自带的 riscv32-unknown-elf-gcc。

TOOLCHAIN ?= 
PREFIX    := riscv32-unknown-elf-
CC        := "$(TOOLCHAIN)/$(PREFIX)gcc"
LD        := "$(TOOLCHAIN)/$(PREFIX)ld"
OBJCOPY   := "$(TOOLCHAIN)/$(PREFIX)objcopy"
READELF   := "$(TOOLCHAIN)/$(PREFIX)readelf"
NM        := "$(TOOLCHAIN)/$(PREFIX)nm"

MARCH := rv32imac_zicsr_zifencei_zba_zbb_zbc_zbs
MABI  := ilp32

VARIANT ?= 60M
SRC     ?= ../SW_DP_GPIO_ASM_$(VARIANT).S

BUILD := build
OBJ   := $(BUILD)/swd_gpio_asm_$(VARIANT).o
ELF   := $(BUILD)/swd_gpio_asm_$(VARIANT)_blob.elf
BIN   := $(BUILD)/swd_gpio_asm_$(VARIANT)_blob.bin
HDR   := $(BUILD)/swd_gpio_asm_$(VARIANT)_blob.h

ASFLAGS := -march=$(MARCH) -mabi=$(MABI) -Ofast

.PHONY: all clean check
all: $(BIN) $(HDR) check

$(BUILD):
	mkdir $(BUILD)

$(OBJ): $(SRC) | $(BUILD)
	$(CC) $(ASFLAGS) -c $< -o $@

$(ELF): $(OBJ) blob.ld
	$(LD) -T blob.ld -Map $(BUILD)/swd_gpio_asm_$(VARIANT).map -o $@ $(OBJ)

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

# 从符号表生成各函数在 blob 内的偏移头文件,供运行时加载/调用使用
$(HDR): $(ELF)
	@echo "generating $@"
	@$(NM) --defined-only -n $< | awk '\
		BEGIN { n = 0 } \
		$$3 ~ /SWJ_Sequence|SWD_Write|SWD_Read/ { name[n] = $$3; addr[n] = $$1; n++ } \
		END { \
			print "/* Auto-generated. Offsets of SWD functions inside swd_gpio_asm_$(VARIANT)_blob.bin */"; \
			print "#ifndef SWD_GPIO_ASM_$(VARIANT)_BLOB_H"; \
			print "#define SWD_GPIO_ASM_$(VARIANT)_BLOB_H"; \
			for (i = 0; i < n; i++) { \
				split(name[i], a, "_GPIO_ASM_"); \
				printf "#define %s_OFFSET_$(VARIANT) 0x%s\n", toupper(a[1]), addr[i]; \
			} \
			print ""; \
			print "#endif /* SWD_GPIO_ASM_$(VARIANT)_BLOB_H */"; \
		}' > $@

# 校验 blob 中不残留绝对重定位。
# 汇编产物允许出现 PC 相对的 BRANCH/JUMP/ALIGN(静态链接时全部就地解析),
# 绝对重定位(R_RISCV_32/64/HI20/LO12/CALL/JAL/GOT)一旦出现即不可搬运。
check: $(ELF)
	@$(READELF) -r $(OBJ) | grep -E "R_RISCV_(32|64|HI20|LO12|CALL|JAL|GOT)" \
		&& { echo "ERROR: blob 含绝对重定位,不可搬运!"; exit 1; } \
		|| echo "check: no absolute relocations, blob is position-independent"
	@$(READELF) -r $(ELF) | grep -q R_RISCV \
		&& { echo "ERROR: linked elf 仍含重定位!"; exit 1; } \
		|| echo "check: linked elf fully resolved"

clean:
	rm -rf $(BUILD)
