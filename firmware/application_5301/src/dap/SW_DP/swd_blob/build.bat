riscv32-unknown-elf-gcc -march=rv32imac_zicsr_zifencei_zba_zbb_zbc_zbs -mabi=ilp32 -mcmodel=medany -fPIE -ffunction-sections -fdata-sections -Ofast -c ..\SW_DP_GPIO_ASM_SLOW.S -o ./swd_slow.o

riscv32-unknown-elf-gcc -march=rv32imac_zicsr_zifencei_zba_zbb_zbc_zbs -mabi=ilp32 -mcmodel=medany -nostdlib -T .\blob.ld "-Wl,-gc-sections" .\swd_slow.o -o swd_slow.elf

riscv32-unknown-elf-objcopy -O binary --only-section=.fast .\swd_slow.elf swd_slow.bin

@REM riscv32-unknown-elf-objdump -d swd_slow.elf > swd_slow.dis