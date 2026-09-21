set confirm off
set pagination off
target extended-remote localhost:2331
printf "rx.base=%p rx.ch=%d\n", dma_resource_pools[0].base, dma_resource_pools[0].channel
x/24xw &rx_descriptors
p/x ((DMAV2_Type*)0xF00C8000)->CHCTRL[dma_resource_pools[0].channel].LLPOINTER
p/x ((DMAV2_Type*)0xF00C8000)->CHCTRL[dma_resource_pools[0].channel].TRANSIZE
p/x ((DMAV2_Type*)0xF00C8000)->CHCTRL[dma_resource_pools[0].channel].DSTADDR
x/4xw &rx_active_desc
detach
quit
