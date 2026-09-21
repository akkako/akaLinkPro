set confirm off
set pagination off
set architecture riscv:rv32
target extended-remote localhost:2331
monitor reset
load
monitor reset
break main
continue
info registers pc
bt
monitor go
detach
quit
