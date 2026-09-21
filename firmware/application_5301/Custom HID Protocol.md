# Custom HID 通信协议

## HID 数据包

固定长度64字节

Byte[0x00] = Report ID
Byte[0x01] = Data Length（包括 Command type 和后面的数据长度）
Byte[0x02] = Command type
Byte[0x03-0x3F] = Command data（可选）

- 主机下发 request 使用 Report ID 0x01，设备回应 response 使用 Report ID 0x02。
- Data Length = Command type 字节数(1) + 后续有效数据字节数。

## 指令类型

1. 获取配置指令 0x01
   主机发送 request
   Byte[0x00] = 0x01 // Report ID
   Byte[0x01] = 0x01 // Data Length
   Byte[0x02] = 0x01 // Command type
   设备回应 response
   Byte[0x00] = 0x02 // Report ID
   Byte[0x01] = 0x07 // Data Length
   Byte[0x02] = 0x01 // Command type
   Byte[0x03] = 输出模式 0x00 - SWD + VCOM; 0x01 - SWD + JTAG
   Byte[0x04] = 5V输出模式 0x00 - Disable; 0x01 - Enable
   Byte[0x05] = 时钟加速模式 0x00 - Disable; 0x01 - Enable
   Byte[0x06] = LED1 显示模式 0x01 - 0x09（见配置说明）
   Byte[0x07] = LED2 显示模式 0x01 - 0x09（见配置说明）
   Byte[0x08] = 外部参考设定值 低八位（单位 mV）
   Byte[0x09] = 外部参考设定值 高八位（单位 mV，范围 1800 - 5000）
   设备回应代表成功

2. 设置配置指令 0x02
   主机发送 request
   Byte[0x00] = 0x01 // Report ID
   Byte[0x01] = 0x07 // Data Length
   Byte[0x02] = 0x02 // Command type
   Byte[0x03] = 输出模式 0x00 - SWD + VCOM; 0x01 - SWD + JTAG
   Byte[0x04] = 5V输出模式 0x00 - Disable; 0x01 - Enable
   Byte[0x05] = 时钟加速模式 0x00 - Disable; 0x01 - Enable
   Byte[0x06] = LED1 显示模式 0x01 - 0x09
   Byte[0x07] = LED2 显示模式 0x01 - 0x09
   Byte[0x08] = 外部参考设定值 低八位（单位 mV）
   Byte[0x09] = 外部参考设定值 高八位（单位 mV，范围 1800 - 5000）
   设备回应 response
   Byte[0x00] = 0x02 // Report ID
   Byte[0x01] = 0x01 // Data Length
   Byte[0x02] = 0x02 // Command type
   设备回应代表成功
   说明：本指令只写入内存并立即生效，掉电不保存；需要掉电保存请随后发送保存配置指令 0x04。
   越界的 LED 模式会被钳制为 0x09（常灭），越界的外部参考设定值会被钳制到 1800 - 5000。

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
   说明：返回 PB10 分压点经 x2 还原后的外部参考电压（ADC 量程 3.3V）。

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
   说明：将当前配置写入 flash 持久化保存，见“配置持久化”章节。

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

10. 获取硬件生产日期指令 0x15
   主机发送 request
   Byte[0x00] = 0x01 // Report ID
   Byte[0x01] = 0x01 // Data Length
   Byte[0x02] = 0x15 // Command type
   设备回应 response
   Byte[0x00] = 0x02 // Report ID
   Byte[0x01] = 0x15 // Data Length
   Byte[0x02] = 0x15 // Command type
   Byte[0x03-0x15] = 日期字符串（19 字节，如 "2026-08-06"）
   Byte[0x16] = \0
   设备回应代表成功

11. 获取固件编译日期指令 0x16
   主机发送 request
   Byte[0x00] = 0x01 // Report ID
   Byte[0x01] = 0x01 // Data Length
   Byte[0x02] = 0x16 // Command type
   设备回应 response
   Byte[0x00] = 0x02 // Report ID
   Byte[0x01] = 0x15 // Data Length
   Byte[0x02] = 0x16 // Command type
   Byte[0x03-0x15] = 日期字符串（19 字节）
   Byte[0x16] = \0
   设备回应代表成功

12. 获取Bootloader编译日期指令 0x17
   主机发送 request
   Byte[0x00] = 0x01 // Report ID
   Byte[0x01] = 0x01 // Data Length
   Byte[0x02] = 0x17 // Command type
   设备回应 response
   Byte[0x00] = 0x02 // Report ID
   Byte[0x01] = 0x15 // Data Length
   Byte[0x02] = 0x17 // Command type
   Byte[0x03-0x15] = 日期字符串（19 字节）
   Byte[0x16] = \0
   设备回应代表成功

13. 设备复位指令 0xFE
   主机发送 request
   Byte[0x00] = 0x01 // Report ID
   Byte[0x01] = 0x01 // Data Length
   Byte[0x02] = 0xFE // Command type
   设备复位，不会回复，此时连接断开

14. 进入DFU模式设置指令 0xFF
   主机发送 request
   Byte[0x00] = 0x01 // Report ID
   Byte[0x01] = 0x01 // Data Length
   Byte[0x02] = 0xFF // Command type
   设备复位进入DFU模式，不会回复，此时连接断开

## 配置说明

1. 输出模式
0x00 - SWD + VCOM 此时 JTAG 功能不可用，TDI 和 TDO 用于 UART
0x01 - SWD + JTAG 此时 VCOM 功能不可用，TDI 和 TDO 用于 JTAG

2. 5V输出模式
0x00 - 关闭 5V 对外输出
0x01 - 开启 5V 对外输出，可以用于为外接隔离器模块或目标板供电
注意：该引脚同时给板载电平转换供电，关闭后 JTAG/SWD 电平转换可能无法工作；出厂默认开启。

3. 时钟加速模式
0x00 - 关闭时钟加速模式，此时 DAP_SWJ_Clock 指令将按照设定频率向下取整设置 SWD 和 JTAG 输出频率，适用于 openocd 等上位机
0x01 - 开启时钟加速模式，此时 DAP_SWJ_Clock 指令将按照设定频率x10向下取整设置 SWD 和 JTAG 输出频率，适用于 Keil MDK 上位机

4. LED 显示模式（LED1 = PB11 蓝色，LED2 = PB12 黄色，高电平点亮）
可分别为两个 LED 配置 0x01 - 0x09 中任意一种模式，两个 LED 可相同也可不同：

0x01 - DAP RUNNING 状态：RUNNING=1 则亮，=0 则灭
0x02 - DAP CONNECT 状态：CONNECT=1 则亮，=0 则灭
0x03 - DAP 状态：RUNNING 与 CONNECT 任一为 1 则以 5Hz（200ms 周期，亮/灭各 100ms）闪烁，两者都为 0 则常亮
0x04 - 调试器电源状态：上电常亮
0x05 - 外部参考电源状态：ADC 检测到外部参考电压高于设定值的 90% 时点亮，低于 85% 时熄灭（5% 滞回）
0x06 - CDC 串口 TX 状态（调试器 UART 向外发数据）：有数据发送时点亮，最低点亮 50ms，持续发送则常亮
0x07 - CDC 串口 RX 状态（调试器 UART 接收数据）：有数据接收时点亮，最低点亮 50ms，持续接收则常亮
0x08 - CDC 串口 TX 或 RX 状态：TX 或 RX 任一触发，逻辑同上
0x09 - 常灭

出厂默认：LED1 = 0x04，LED2 = 0x05。

5. 外部参考电压设定值（单位 mV，范围 1800 - 5000）
用于 LED 显示模式 0x05 的判定阈值，对应经 x2 还原后的外部参考电压。出厂默认 3300mV。

## 配置持久化

- 设置配置指令（0x02）只修改内存中的配置并立即生效，掉电丢失。
- 保存配置指令（0x04）通过 **EasyFlash（ENV, NG 模式）** 写入 QSPI NOR flash，
  具备磨损平衡与掉电保护。
- 存储位置：APP 固件区尾部保留的两个 4K 扇区（`0x800FE000` 与 `0x800FF000`），
  不占用 Bootloader 区（`0x80000000` - `0x8001FFFF`）。
- ENV key 为 `"cfg"`，value 为整个 `api_param_t`；上电自动加载，校验失败则写回出厂默认。
- 常规 APP 升级（J-Link 或 dfu-util）不会覆盖该区域。
- 详见 `Flash_Memory_Map.md` 与 `Firmware_Integrity_Plan.md`。

## 固件元数据（版本 / CRC）

- 版本、编译时间、描述、硬件版本等由构建后 `firmware/tools/pack.py` 注入：
  - **APP**：`0x80020000` 起 256 B 头（签名 + 长度 + CRC32 + 版本 + 编译时间 + 描述），代码入口 `0x80020100`；
  - **Bootloader**：`0x8001F000` 起 256 B 信息块（版本 + 编译时间 + 硬件版本 + 生产日期）。
- 版本单一来源：`firmware/version.json`；编译时间取打包时刻。
- Bootloader 启动时校验 APP 头（签名 + 长度 + CRC32），失败则停留在 DFU 模式。
- 指令 `0x12`–`0x17` 即从上述固定地址读取返回。

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
