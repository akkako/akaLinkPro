set confirm off
set pagination off
target extended-remote localhost:2331
p/x s_dma_mngr_ctx.channels[0][0].tc_cb
p/x s_dma_mngr_ctx.channels[0][1].tc_cb
x/8xw &rx_descriptors
x/6xw &uart_rx_buf
p/x sizeof(dma_resource_t)
detach
quit
