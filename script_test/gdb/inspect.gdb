set confirm off
set pagination off
target extended-remote localhost:2331
p/x config_uart
p/x config_uart_transfer
p/x s_uart2_com_mode
p/x g_uart_tx_transfer_length
p/x rx_active_desc
p/x rb_write_pos
p/x HPM_UART2->LCR
p/x HPM_UART2->DLL
p/x HPM_UART2->DLM
p/x HPM_IOC->PAD[IOC_PAD_PA08].FUNC_CTL
p/x HPM_IOC->PAD[IOC_PAD_PA09].FUNC_CTL
p/x HPM_IOC->PAD[IOC_PAD_PB13].FUNC_CTL
x/16xw 0xF0048000
detach
quit
