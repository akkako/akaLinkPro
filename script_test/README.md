# script_test

akaLinkPro (HPM5301) 固件测试脚本集合。这些脚本用于验证 **UART2 ↔ CDC 串口桥**、
**DAP 模式切换**（SWD/空闲 = UART2，JTAG = FGPIO）以及设备枚举状态。

## 依赖

- Python 3.11 + `pyserial`、`pyusb`（`pip install pyserial pyusb`）
- Windows 上需已安装 libusb/WinUSB 驱动（pyusb 后端）
- 硬件：将 **PA08(TXD) 与 PA09(RXD) 短接**（串口回环）
- 设备运行 `application_5301` 固件，并在系统中枚举出 CDC 串口（COMx）

COM 端口号在不同机器上会变化，脚本支持通过参数传入，默认 `COM75`。

## 脚本说明

| 文件 | 用途 |
| --- | --- |
| `uart_loopback.py` | 串口回环压力测试：逐字节回环 + 大块随机数据流，覆盖 115200~2Mbps |
| `diag_loopback.py` | 单次大流量回环并定位首个错误位置/零区块，用于诊断 DMA 边界问题 |
| `test_modeswitch.py` | 通过 CMSIS-DAP 发 `DAP_Connect`(JTAG/SWD)/`DAP_Disconnect`，验证引脚复用切换 |
| `list_usb.ps1` | 列出 `VID_0D28` 相关 USB 设备与 CDC 端口（状态检查） |
| `hold_port.py` | 打开串口并保持若干秒，便于用 J-Link/GDB 在线检查运行状态 |

### 运行示例

```bat
python script_test\uart_loopback.py COM75
python script_test\diag_loopback.py COM75 115200 32768
python script_test\test_modeswitch.py COM75
powershell -ExecutionPolicy Bypass -File script_test\list_usb.ps1
python script_test\hold_port.py COM75 8
```

## gdb/ — 在线调试检查脚本（配合 JLink GDB Server）

先启动 GDB Server（或用 `firmware/application_5301/gdb_server.bat`）：

```
JLinkGDBServerCL.exe -device HPM5301xEGx -if JTAG -speed 4000 -port 2331 -nogui -singlerun
```

然后：

```bat
riscv32-unknown-elf-gdb -batch -x script_test\gdb\<script>.gdb ^
  firmware\application_5301\build_dfu\output\akaLinkPro_App.elf
```

| 文件 | 用途 |
| --- | --- |
| `dbg_app.gdb` | 连接/复位/load/命中断点 main（与 VSCode launch 流程等价） |
| `dbg_test.gdb` | 连接后 reset/load/break main/查看寄存器 |
| `inspect*.gdb` | 读取 UART2 寄存器、DMA 通道、`dma_resource_pools`、引脚 FUNC_CTL 等 |

> 注意：`inspect*.gdb` 中的寄存器地址/符号对应 HPM5301 + 本工程，改动后需同步。

## jlink/ — J-Link 命令行测试片段

用法：`JLink.exe -NoGui 1 -ExitOnError 1 -CommanderScript script_test\jlink\<script>.jlink`

| 文件 | 用途 |
| --- | --- |
| `jl_probe.jlink` | `ShowEmuList`，列出已连接的 J-Link |
| `jl_min.jlink` | 最小 JTAG 连接测试 |
| `jl_test3.jlink` | JTAG 自动探测 + 连接 + 读内存 |
| `jl_read.jlink` | 读 flash（APP 签名 `HPM!`） |
| `jl_pins.jlink` | 读 PA08/PA09/PB13 的 IOC FUNC_CTL 与 GPIO 输出状态 |
| `jl_load.jlink` | `erase` + `loadfile` 示例（路径需按需修改） |
| `jl_flashinfo.jlink` / `jl_help.jlink` / `jl_test.jlink` / `jl_test2.jlink` | 早期调试片段 |

## 关键结论（回归基线）

- 回环压力测试：115200 / 460800 / 921600 / 1M / 2Mbps **全部通过**。
- 模式切换：空闲回环 OK → JTAG 下 COM 无回显（正常）→ SWD 恢复 OK → Disconnect 恢复 OK。
- 引脚状态：PB13 `FUNC_CTL=0` 且输出高（5V_EN 开）；空闲/SWD 下 PA08/PA09 `FUNC_CTL=2`(UART2)。
