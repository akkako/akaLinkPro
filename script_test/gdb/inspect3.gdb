set confirm off
set pagination off
target extended-remote localhost:2331
printf "rx.base=%p rx.ch=%d tx.base=%p tx.ch=%d\n", dma_resource_pools[0].base, dma_resource_pools[0].channel, dma_resource_pools[1].base, dma_resource_pools[1].channel
x/8xw &dma_resource_pools
p/x ((UART_Type*)0xF0048000)->LSR
p/x ((UART_Type*)0xF0048000)->LCR
p/x ((UART_Type*)0xF0048000)->CFG
p/x ((UART_Type*)0xF0048000)->OSCR
printf "--- dma ch0 ---\n"
p/x ((DMAV2_Type*)0xF00C8000)->CHCTRL[0].CTRL
p/x ((DMAV2_Type*)0xF00C8000)->CHCTRL[0].TRANSIZE
p/x ((DMAV2_Type*)0xF00C8000)->CHCTRL[0].SRCADDR
p/x ((DMAV2_Type*)0xF00C8000)->CHCTRL[0].DSTADDR
printf "--- dma ch1 ---\n"
p/x ((DMAV2_Type*)0xF00C8000)->CHCTRL[1].CTRL
p/x ((DMAV2_Type*)0xF00C8000)->CHCTRL[1].TRANSIZE
p/x ((DMAV2_Type*)0xF00C8000)->CHCTRL[1].SRCADDR
p/x ((DMAV2_Type*)0xF00C8000)->CHCTRL[1].DSTADDR
detach
quit
