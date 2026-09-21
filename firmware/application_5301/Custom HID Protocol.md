# Custom HID 通信协议

## HID 数据包

固定长度64字节

Byte[0x00] = Report ID
Byte[0x01] = Data Length（包括 Command type 和后面的数据长度）
Byte[0x02] = Command type
Byte[0x03-0x3F] = Command data（可选）

## 指令类型

1. 获取配置指令 0x01
   主机发送 request
   Byte[0x00] = 0x01 // Report ID
   Byte[0x01] = 0x01 // Data Length
   Byte[0x02] = 0x01 // Command type
   设备回应 response
   Byte[0x00] = 0x02 // Report ID
   Byte[0x01] = 0x05 // Data Length
   Byte[0x02] = 0x01 // Command type
   Byte[0x03] = 输出模式 0x00 - SWD + VCOM; 0x01 - SWD + JTAG
   Byte[0x04] = SWD模拟模式 0x00 - SPI; 0x01 - GPIO
   Byte[0x05] = 5V输出模式 0x00 - Disable; 0x01 - Enable
   Byte[0x06] = 时钟加速模式 0x00 - Disable; 0x01 - Enable
   设备回应代表成功

2. 设置配置指令 0x02
   主机发送 request
   Byte[0x00] = 0x01 // Report ID
   Byte[0x01] = 0x05 // Data Length
   Byte[0x02] = 0x02 // Command type
   Byte[0x03] = 输出模式 0x00 - SWD + VCOM; 0x01 - SWD + JTAG
   Byte[0x04] = SWD模拟模式 0x00 - SPI; 0x01 - GPIO
   Byte[0x05] = 5V输出模式 0x00 - Disable; 0x01 - Enable
   Byte[0x06] = 时钟加速模式 0x00 - Disable; 0x01 - Enable
   设备回应 response
   Byte[0x00] = 0x02 // Report ID
   Byte[0x01] = 0x01 // Data Length
   Byte[0x02] = 0x02 // Command type
   设备回应代表成功

3. 获取 Target 电压获取指令 0x03
   主机发送 request
   Byte[0x00] = 0x01 // Report ID
   Byte[0x01] = 0x01 // Data Length
   Byte[0x02] = 0x03 // Command type
   设备回应 response
   Byte[0x00] = 0x02 // Report ID
   Byte[0x01] = 0x03 // Data Length
   Byte[0x02] = 0x03 // Command type
   Byte[0x03] = 电压数据低八位
   Byte[0x04] = 电压数据高八位（电压单位为mV）
   设备回应代表成功

4. 保存当前配置设置指令 0x04
   主机发送 request
   Byte[0x00] = 0x01 // Report ID
   Byte[0x01] = 0x01 // Data Length
   Byte[0x02] = 0x04 // Command type
   设备回应 response
   Byte[0x00] = 0x02 // Report ID
   Byte[0x01] = 0x01 // Data Length
   Byte[0x02] = 0x04 // Command type
   设备回应代表成功

5. 获取型号指令 0x10
   主机发送 request
   Byte[0x00] = 0x01 // Report ID
   Byte[0x01] = 0x01 // Data Length
   Byte[0x02] = 0x10 // Command type
   设备回应 response
   Byte[0x00] = 0x02 // Report ID
   Byte[0x01] = 0x13 // Data Length
   Byte[0x02] = 0x10 // Command type
   Byte[0x03] = 'a'
   Byte[0x04] = 'k'
   Byte[0x05] = 'a'
   Byte[0x06] = 'L'
   Byte[0x07] = 'i'
   Byte[0x08] = 'n'
   Byte[0x09] = 'k'
   Byte[0x0A] = ' '
   Byte[0x0B] = 'C'
   Byte[0x0C] = 'M'
   Byte[0x0D] = 'S'
   Byte[0x0E] = 'I'
   Byte[0x0F] = 'S'
   Byte[0x10] = '-'
   Byte[0x11] = 'D'
   Byte[0x12] = 'A'
   Byte[0x13] = 'P'
   Byte[0x14] = \0
   设备回应代表成功

6. 获取序列号指令 0x11
   主机发送 request
   Byte[0x00] = 0x01 // Report ID
   Byte[0x01] = 0x01 // Data Length
   Byte[0x02] = 0x11 // Command type
   设备回应 response
   Byte[0x00] = 0x02 // Report ID
   Byte[0x01] = 0x0E // Data Length
   Byte[0x02] = 0x11 // Command type
   Byte[0x03] = SN 第 1 位 ACSII
   Byte[0x04] = SN 第 2 位 ACSII
   Byte[0x05] = SN 第 3 位 ACSII
   Byte[0x06] = SN 第 4 位 ACSII
   Byte[0x07] = SN 第 5 位 ACSII
   Byte[0x08] = SN 第 6 位 ACSII
   Byte[0x09] = SN 第 7 位 ACSII
   Byte[0x0A] = SN 第 8 位 ACSII
   Byte[0x0B] = SN 第 9 位 ACSII
   Byte[0x0C] = SN 第 10 位 ACSII
   Byte[0x0D] = SN 第 11 位 ACSII
   Byte[0x0E] = SN 第 12 位 ACSII
   Byte[0x0F] = \0
   设备回应代表成功

7. 获取硬件版本号 0x12
   主机发送 request
   Byte[0x00] = 0x01 // Report ID
   Byte[0x01] = 0x01 // Data Length
   Byte[0x02] = 0x12 // Command type
   设备回应 response
   Byte[0x00] = 0x02 // Report ID
   Byte[0x01] = 0x06 // Data Length
   Byte[0x02] = 0x12 // Command type
   Byte[0x03] = HW 第 1 位 ACSII
   Byte[0x04] = HW 第 2 位 ACSII
   Byte[0x05] = HW 第 3 位 ACSII
   Byte[0x06] = HW 第 4 位 ACSII
   Byte[0x07] = \0
   设备回应代表成功

8. 获取固件版本号 0x13
   主机发送 request
   Byte[0x00] = 0x01 // Report ID
   Byte[0x01] = 0x01 // Data Length
   Byte[0x02] = 0x13 // Command type
   设备回应 response
   Byte[0x00] = 0x02 // Report ID
   Byte[0x01] = 0x06 // Data Length
   Byte[0x02] = 0x13 // Command type
   Byte[0x03] = FW 第 1 位 ACSII
   Byte[0x04] = FW 第 2 位 ACSII
   Byte[0x05] = FW 第 3 位 ACSII
   Byte[0x06] = FW 第 4 位 ACSII
   Byte[0x07] = \0
   设备回应代表成功

9. 获取Bootloader版本号 0x14
   主机发送 request
   Byte[0x00] = 0x01 // Report ID
   Byte[0x01] = 0x01 // Data Length
   Byte[0x02] = 0x14 // Command type
   设备回应 response
   Byte[0x00] = 0x02 // Report ID
   Byte[0x01] = 0x06 // Data Length
   Byte[0x02] = 0x14 // Command type
   Byte[0x03] = BL 第 1 位 ACSII
   Byte[0x04] = BL 第 2 位 ACSII
   Byte[0x05] = BL 第 3 位 ACSII
   Byte[0x06] = BL 第 4 位 ACSII
   Byte[0x07] = \0
   设备回应代表成功

10. 设备复位指令 0xFE
   主机发送 request
   Byte[0x00] = 0x01 // Report ID
   Byte[0x01] = 0x01 // Data Length
   Byte[0x02] = 0xFE // Command type
   设备复位，不会回复，此时连接断开

11. 进入DFU模式设置指令 0xFF
   主机发送 request
   Byte[0x00] = 0x01 // Report ID
   Byte[0x01] = 0x01 // Data Length
   Byte[0x02] = 0xFF // Command type
   设备复位进入DFU模式，不会回复，此时连接断开

## 配置说明

1. 输出模式
0x00 - SWD + VCOM 此时 JTAG 功能不可用，TDI 和 TDO 用于 UART
0x01 - SWD + JTAG 此时 VCOM 功能不可用，TDI 和 TDO 用于 JTAG

2. SWD模拟模式
0x00 - SPI 模拟 SWD 时序，能够达到较快速度，可能会出现兼容性问题
0x01 - GPIO 模拟 SWD 时序，速度较慢，但是较为稳定

3. 5V输出模式
0x00 - 关闭 5V 对外输出
0x01 - 开启 5V 对外输出，可以用于为外接隔离器模块或目标板供电

4. 时钟加速模式
0x00 - 关闭时钟加速模式，此时 DAP_SWJ_Clock 指令将按照设定频率向下取整设置 SWD 和 JTAG 输出频率，适用于 openocd 等上位机
0x01 - 开启时钟加速模式，此时 DAP_SWJ_Clock 指令将按照设定频率x10向下取整设置 SWD 和 JTAG 输出频率，适用于 Keil MDK 上位机

## WebUI 配置界面

点击连接按钮尝试连接 HID 设备
弹出 HID 设备选择框，选择 akaLink CMSIS-DAP 设备
进行连接，如果连接失败显示错误提示信息
连接成功，读取型号，序列号，配置信息，并将数据显示在页面上
此时可以修改配置，修改完配置不立即写入
点击保存配置按钮，将配置写入并保存
点击复位按钮，进行复位操作
点击进入DFU模式按钮，进入DFU模式

需要实时监测 HID 设备的连接状态，如果丢失连接要及时响应，清除显示的数据
在未连接状态下，只有连接按钮可用，其他按钮不可用，配置项目不可选

每个配置项和按键，当鼠标指上去的时候需要有提示信息