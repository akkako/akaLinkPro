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
| `uart_loopback_hs.py` | 高速回环测试：2Mbps~11.25Mbps，含吞吐量与线速效率统计 |
| `uart_loopback_common.py` | 常见速率回环详表：请求/实际波特率、误差、耗时、吞吐、线速效率 |
| `diag_loopback.py` | 单次大流量回环并定位首个错误位置/零区块，用于诊断 DMA 边界问题 |
| `test_modeswitch.py` | 通过 CMSIS-DAP 发 `DAP_Connect`(JTAG/SWD)/`DAP_Disconnect`，验证引脚复用切换 |
| `uart_stress_loop.py` | 定时长回环压测（发一块读一块），统计 errors/late/最大延迟，可区分丢失与延迟 |
| `uart_stress_stream.py` | 连续流回环压测（读线程 + 不丢包比对），用于与 SWD 并发时做满负荷验证 |
| `list_usb.ps1` | 列出 `VID_0D28` 相关 USB 设备与 CDC 端口（状态检查） |
| `hold_port.py` | 打开串口并保持若干秒，便于用 J-Link/GDB 在线检查运行状态 |
| `swd/run_benchmark.py` | 生成指定 `adapter speed`/`iterations` 的 OpenOCD SWD 读写校验并运行 |

### 运行示例

```bat
python script_test\uart_loopback.py COM75
python script_test\uart_loopback_hs.py COM75 262144
python script_test\uart_loopback_common.py COM75
python script_test\diag_loopback.py COM75 115200 32768
python script_test\test_modeswitch.py COM75
powershell -ExecutionPolicy Bypass -File script_test\list_usb.ps1
python script_test\hold_port.py COM75 8
```

### 串口速率上限与就近取整（HPM5301）

- 驱动器公式：`baud = uart_clk / (div * osc)`，`osc` 为 8~30 的偶数。
- **默认（符合手册）**：UART2 时钟 = `PLL0CLK0(720MHz)/9 = 80MHz`（UART 输入时钟手册上限），
  硬件/软件上限 = `uart_clk/8 = 10 Mbps`（`UART2_CLK_DIV=9`、`UART2_MAX_BAUDRATE=10000000`）。
- **超频选项**：`CMakeLists.txt` 里取消注释 `sdk_compile_definitions(-DUART2_OVERCLOCK=1)`，
  则 `720/4 = 180MHz`、上限 22.5 Mbps，且 11.25/15/18 Mbps 可精确生成。
  **注意：180MHz 超出手册限制，不保证所有芯片稳定，仅供测试。**
- 无法精确生成的按**就近取整**（`uart2_round_baudrate()`）。
- 实际写入的波特率存于 `g_uart2_applied_baud`（可用 J-Link/GDB 读取核对）。
- 实测吞吐（默认 80MHz）：10M→~972KB/s（线速效率 99.6%）；>10M 请求被钳到 10M。
  超频 180MHz：22.5M→~2108KB/s。

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

## CDC + SWD 同时满载

SWD 压测（`swd/run_benchmark.py`，目标可为 STM32F1，`adapter speed` 20/36/45/60MHz）
与 CDC 连续流回环（`uart_stress_stream.py COM75 9000000 <秒>`）同时运行。

实测（SWD 1000 轮 + CDC 9Mbps 连续流）：

| SWD 速度 | CDC 速率 | SWD 校验 | CDC 连续流 |
| --- | --- | --- | --- |
| 20 MHz | 9 Mbps | 1000/1000 PASS | 8/8 OK（每轮 ~7MB） |
| 20 MHz | 10 Mbps | 1000/1000 PASS | 8/8 OK |
| 20 MHz | 22.5 Mbps | 1000/1000 PASS | 16/16 OK（2 轮，~230MB） |
| 36 MHz | 9 Mbps | 1000/1000 PASS | 全 OK |
| 45 MHz | 9 Mbps | 1000/1000 PASS | 全 OK |
| 60 MHz | 9 Mbps | 1000/1000 PASS | 全 OK |
| 60 MHz | 22.5 Mbps | 1000/1000 PASS | 4/4 OK |

说明：SWD 20MHz 的临界区抖动最大；即使 `20MHz SWD + 22.5Mbps CDC`（双向极限）
两轮 16/16 也全部干净，无掉/重数据。偶发失败需先排查硬件接触。

结论：SWD 20~60MHz 与 CDC 9Mbps **同时满载无掉数据/无重复**。
（关键修复：给 `g_uartrx` 的 DMA-TC 生产者补齐临界区；并用 GPTMR 定时器驱动 RX flush，
不依赖 IDLE/满缓冲中断。）

RX flush 定时器周期**按波特率动态调整**（目标每次约 512 字节，clamp 到 200us~10ms）：
低波特率时降低中断频率。实测 GPTMR RLD：`9600 → 10ms`、`9M → 568us`。

## 关键结论（回归基线）

- 回环压力测试：115200 / 460800 / 921600 / 1M / 2Mbps **全部通过**。
- 常见速率（`uart_loopback_common.py`）：9600~9M **全部通过**；
  6M→5.625M、8M→7.5M、10M→9M、11.25M→9M（9M 软件上限 + 就近取整）；高速线速效率 ~99.7%。
- 高速回环（1MB×多次，稳定）：2M~9M **全部通过**，线速效率 ~100%。
- 模式切换：空闲回环 OK → JTAG 下 COM 无回显（正常）→ SWD 恢复 OK → Disconnect 恢复 OK。
- 引脚状态：PB13 `FUNC_CTL=0` 且输出高（5V_EN 开）；空闲/SWD 下 PA08/PA09 `FUNC_CTL=2`(UART2)。
- RX 采用 **DMAV2 infinite-loop 圆形缓冲**（无 disable/restart），消除了重启边界上的重复字节。
