# akaLink Pro Pinout define

| Pin name | IO name | alt function | description                     | Init state             |
| -------- | ------- | ------------ | ------------------------------- | ---------------------- |
| ISP_TX   | PA00    | UART0.TXD    | ISP and debug print serial port |                        |
| ISP_RX   | PA01    | UART0.RXD    | ISP and debug print serial port |                        |
| BOOT0    | PA02    | None         | Strapping pin                   |                        |
| BOOT1    | PA03    | None         | Strapping pin                   |                        |
| DBG_JTDO | PA04    | JTAG         | Debug port                      |                        |
| DBG_JTDI | PA05    | JTAG         | Debug port                      |                        |
| DBG_JTCK | PA06    | JTAG         | Debug port                      |                        |
| DBG_JTMS | PA07    | JTAG         | Debug port                      |                        |
| JTDI_TXD | PA08    | UART2.TXD    | VCOM port                       |                        |
| JTDI_RXD | PA09    | UART2.RXD    | VCOM port                       |                        |
| DFU_PIN  | PA10    | GPIO0 Input | DFU mode entry detect           | Pull-down GPIO0 Input |
| USB_D+   | PA24    | USB          | USB communication pin           |                        |
| USB_D-   | PA25    | USB          | USB communication pin           |                        |
| nRESET   | PA26    | GPIO0 Output |                                 | GPIO0 Output Low       |
| JTCK     | PA27    | FGPIO Output |                                 |                        |
| JTMS_IN  | PA28    | FGPIO Input  |                                 |                        |
| JTMS_OUT | PA29    | FGPIO Output |                                 |                        |
| JTMS_DIR | PA30    | FGPIO Output |                                 |                        |
| JTRST    | PA31    | GPIO0 Output |                                 |                        |
| SEL0     | PB08    | GPIO0 Input | Device hardware type detect     | Pull-up GPIO0 Input    |
| SEL1     | PB09    | GPIO0 Input | Device hardware type detect     | Pull-up GPIO0 Input    |
| ADC_VREF | PB10    | ADC0.IN2     | External Vref detect            |                        |
| LED1     | PB11    | GPIO0 Output |                                 | GPIO0 Output Low       |
| LED2     | PB12    | GPIO0 Output |                                 | GPIO0 Output Low       |
| 5V_EN    | PB13    | GPIO0 Output |                                 | GPIO0 Output Low       |
| AUX_RXD  | PB14    | UART3.RXD    | Auxiliary COM port              |                        |
| AUX_TXD  | PB15    | UART3.TXD    | Auxiliary COM port              |                        |
| Unused   | PY00    | None         |                                 | GPIO0 Output Low       |
| Unused   | PY01    | None         |                                 | GPIO0 Output Low       |
