set confirm off
set pagination off
target extended-remote localhost:2331
p/x s_dma_mngr_ctx.dma_instance[0].base
p/x s_dma_mngr_ctx.channels[0][0].is_allocated
p/x s_dma_mngr_ctx.channels[0][1].is_allocated
p/x s_dma_mngr_ctx.channels[0][2].is_allocated
p/x s_dma_mngr_ctx.channels[0][3].is_allocated
x/16xw &s_dma_mngr_ctx
detach
quit
