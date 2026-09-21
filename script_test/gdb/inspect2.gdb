set confirm off
set pagination off
target extended-remote localhost:2331
printf "UART2 regs @0xF0048000\n"
x/16xw 0xF0048000
p/x ((UART_Type*)0xF0048000)->LCR
p/x ((UART_Type*)0xF0048000)->DLL
p/x ((UART_Type*)0xF0048000)->DLM
p/x ((UART_Type*)0xF0048000)->FCR
p/x ((UART_Type*)0xF0048000)->IER
p/x ((UART_Type*)0xF0048000)->LSR
p/x ((UART_Type*)0xF0048000)->CFG
p/x config_uart_transfer
p/x g_uart_tx_transfer_length
p/x dma_resource_pools[0].channel
p/x dma_resource_pools[1].channel
printf "DMA ch runtime\n"
p/x ((DMA_Type*)0xF00C8000)->CHCTRL[dma_resource_pools[0].channel].CTRL
p/x ((DMA_Type*)0xF00C8000)->CHCTRL[dma_resource_pools[0].channel].TRANSIZE
p/x ((DMA_Type*)0xF00C8000)->CHCTRL[dma_resource_pools[1].channel].CTRL
p/x ((DMA_Type*)0xF00C8000)->CHCTRL[dma_resource_pools[1].channel].TRANSIZE
p/x ((DMA_Type*)0xF00C8000)->CHCTRL[dma_resource_pools[1].channel].SRCADDR
p/x ((DMA_Type*)0xF00C8000)->CHCTRL[dma_resource_pools[1].channel].DSTADDR
detach
quit
