set confirm off
set pagination off
target extended-remote localhost:2331
printf "PB13 FUNC_CTL=0x%x (expect 0)\n", HPM_IOC->PAD[IOC_PAD_PB13].FUNC_CTL
printf "PB13 OE=0x%x DO=0x%x\n", HPM_GPIO0->OE[GPIO_GET_PORT_INDEX(IOC_PAD_PB13)].VALUE, HPM_GPIO0->DO[GPIO_GET_PORT_INDEX(IOC_PAD_PB13)].VALUE
printf "PA08 TXD FUNC_CTL=0x%x expect=0x%x\n", HPM_IOC->PAD[IOC_PAD_PA08].FUNC_CTL, IOC_PA08_FUNC_CTL_UART2_TXD
printf "PA09 RXD FUNC_CTL=0x%x expect=0x%x\n", HPM_IOC->PAD[IOC_PAD_PA09].FUNC_CTL, IOC_PA09_FUNC_CTL_UART2_RXD
detach
quit
