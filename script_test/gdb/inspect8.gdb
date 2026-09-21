set confirm off
set pagination off
target extended-remote localhost:2331
printf "PB13 FUNC_CTL=0x%x (expect 0)\n", ((IOC_Type*)0xF4040000)->PAD[IOC_PAD_PB13].FUNC_CTL
printf "PB13 OE=0x%x DO=0x%x\n", ((GPIO_Type*)0xF00D0000)->OE[GPIO_GET_PORT_INDEX(IOC_PAD_PB13)].VALUE, ((GPIO_Type*)0xF00D0000)->DO[GPIO_GET_PORT_INDEX(IOC_PAD_PB13)].VALUE
printf "PA08 TXD FUNC_CTL=0x%x expect=0x%x\n", ((IOC_Type*)0xF4040000)->PAD[IOC_PAD_PA08].FUNC_CTL, IOC_PA08_FUNC_CTL_UART2_TXD
printf "PA09 RXD FUNC_CTL=0x%x expect=0x%x\n", ((IOC_Type*)0xF4040000)->PAD[IOC_PAD_PA09].FUNC_CTL, IOC_PA09_FUNC_CTL_UART2_RXD
detach
quit
