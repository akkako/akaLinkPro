---
name: akalinkpro-firmware
description: akaLinkPro (HPM5301 CMSIS-DAP) firmware build, flash and debug guide. Use when building, flashing (J-Link/JTAG or dfu-util), debugging (VSCode GDB / J-Link GDB Server), or editing files under firmware/application_5301 and firmware/bootloader_dfu in this repo.
---

# akaLinkPro 固件构建 / 烧录 / 调试

本仓库是 **HPM5301** 上的 CMSIS-DAP 调试器（akaLinkPro）。
- 主固件：`firmware/application_5301`（`akaLinkPro_App`）
- DFU Bootloader：`firmware/bootloader_dfu`（`akaLinkPro_Boot`）
- `firmware/application_5331` 是另一芯片变体，非本流程重点。

调试探针：**J-Link V11**，串口 `50120677`，**只能走 JTAG**。

---

## 1. 目录结构

```
firmware/
  application_5301/                 主固件 (CMSIS-DAP)
    CMakeLists.txt                  链接 flash_dfu.ld，预留 0x20000 给 bootloader；会生成 .hex
    build.bat                       原 DFU 构建 -> ./build，末尾执行 dfu-util
    build_zcc.bat                   ZCC 工具链 DFU 构建
    build_dfu.bat           [新增]  构建到 ./build_dfu（不调用 dfu-util，生成 .hex）
    program.bat                     仅执行 dfu-util
    flash_jlink.bat         [新增]  build_dfu + JLink 烧录 APP 区（0x80020000）
    gdb_server.bat          [新增]  启动 JLinkGDBServerCL（VSCode 调试用）
    .vscode/               [新增]  tasks.json / launch.json（GDB 调试，git 忽略）
    boards/akaLinkPro/              board.c/h, clock.c/h, pinmux.c/h, akaLinkPro.yaml
    src/                            main.c, usb/, dap/, api/, dfu/
  bootloader_dfu/                   DFU Bootloader
    CMakeLists.txt                  链接 flash_xip.ld，128K flash
    build.bat                       原 flash_xip 构建 -> ./build
    build_xip.bat           [新增]  构建到 ./build_xip（JLink 流程专用）
    flash_jlink.bat         [新增]  build_xip + JLink 烧录 bootloader（0x80000000）
    src/                            main.c, dfu_desc.c, hpm_dfu_trigger.c, boot_port_board_hpm.c
```

构建产物目录 `build` / `build_xip` / `build_dfu` 均已在 `.gitignore` 中忽略。

---

## 2. 内存布局与关键地址

HPM5301，外挂 1MB QSPI NOR，XIP 基址 `0x80000000`。

| 区域 | 地址 | 链接脚本 | 说明 |
| --- | --- | --- | --- |
| Bootloader | `0x80000000 - 0x8001FFFF` (128K) | `flash_xip.ld` | 含 `nor_cfg_option`@0x400、`boot_header`@0x1000、`.start`@0x3000 |
| Application | `0x80020000 - 0x800FFFFF` | `flash_dfu.ld` | `.start` 首 4 字节为 DFU 签名 |
| ILM | `0x00000000` (128K) | — | 向量表 / `.fast` |
| DLM | `0x00080300` | — | data / bss / heap / stack |
| AHB_SRAM | `0xF0400000` (32K) | — | `.ahb_sram` |

- DFU 签名：`0x80020000` 处必须为 `0x48504D21`（`"HPM!"`，`BOARD_DFU_SIGNATURE`）。
- Bootloader 在 `hpm_dfu_check_bootloader_request()` 中检查该签名，有效则跳到 `0x80020004`。
- **APP 必须链接在 `0x80020000`**（bootloader 之后）。放到 0x80000000 会覆盖 bootloader 或启动失败。

---

## 3. 工具链与依赖（当前机器固定路径）

| 用途 | 路径 |
| --- | --- |
| HPM SDK | `D:\_tools\hpm_sdk\hpm_sdk` |
| RISC-V GCC / GDB | `D:\_tools\hpm_sdk\toolchains\rv32imac_zicsr_zifencei_multilib_b_ext-win\bin` |
| python / cmake / ninja | `D:\_tools\hpm_sdk\tools\{python3,cmake\bin,ninja}` |
| J-Link | `C:\Program Files\SEGGER\JLink\JLink.exe`，`JLinkGDBServerCL.exe` |
| dfu-util | 已在 PATH 中 |

GDB：`...\toolchains\...\bin\riscv32-unknown-elf-gdb.exe`

环境变量（各 `build*.bat` 已内置）：
`HPM_SDK_BASE`、`GNURISCV_TOOLCHAIN_PATH`、`HPM_SDK_TOOLCHAIN_VARIANT=gcc`、`BOARD=akaLinkPro`、`HPM_BUILD_TYPE`。

---

## 4. 构建

```bat
:: Bootloader (flash_xip @0x80000000) -> build_xip\
firmware\bootloader_dfu\build_xip.bat

:: App (flash_dfu @0x80020000) -> build_dfu\   (生成 .hex 与 .bin)
firmware\application_5301\build_dfu.bat
```

原有流程保持不变：
- `bootloader_dfu\build.bat` / `application_5301\build.bat`（`./build`，末尾 dfu-util）
- `application_5301\build_zcc.bat`（ZCC 工具链）

---

## 5. 烧录

### 5.1 J-Link 一键烧录（推荐）

```bat
:: 1) 先烧 bootloader（保留 0x80020000 处已有 APP）
firmware\bootloader_dfu\flash_jlink.bat

:: 2) 再烧 APP（只写 0x80020000 起的扇区，保留 bootloader）
firmware\application_5301\flash_jlink.bat
```

两个脚本都会：构建 -> 生成临时 `.jlink` 命令 -> 调用 `JLink.exe -NoGui 1 -ExitOnError 1` -> reset & go。

J-Link 命令模板（直接改 `flash_jlink.bat` 时遵循）：

```
device HPM5301xEGx
si JTAG
jtagconf -1 -1
speed 4000
connect
Sleep 200
loadfile "<绝对路径.hex>"
Sleep 200
r
Sleep 300
go
Sleep 200
Exit
```

### 5.2 DFU 烧录（原流程）

```bat
firmware\application_5301\build.bat
:: 等价于 cmake 构建 + dfu-util -a 0 -E 1 -s 0x80020000:leave -D build\output\akaLinkPro_App.bin
```

APP 自带 DFU runtime 接口，`dfu-util` 会先发 `DFU_DETACH` 触发重启进入 bootloader，再传输。

---

## 6. 调试

### 6.1 VSCode

打开 `firmware/application_5301` 作为工作区，选 **"Debug App (J-Link + GDB)"**（F5）。
`preLaunchTask` 会先执行 `build_dfu.bat`，再后台启动 `gdb_server.bat`，
cppdbg 通过 `localhost:2331` 连接，执行 `monitor reset -> load -> monitor reset`，
然后在 `main` 停下。

- `tasks.json` 用 `Waiting for GDB connection...` 作为后台任务就绪标志。
- `launch.json` 的 `program` 指向 `build_dfu/output/akaLinkPro_App.elf`。
- 调试 APP 时 bootloader 必须在 flash 中（它负责跳转到 APP）。

### 6.2 手动 GDB（命令行）

```bat
:: 终端 1：启动 GDB Server
"C:\Program Files\SEGGER\JLink\JLinkGDBServerCL.exe" ^
  -device HPM5301xEGx -if JTAG -speed 4000 -port 2331 -nogui -singlerun
```

```bat
:: 终端 2
"D:\_tools\hpm_sdk\toolchains\rv32imac_zicsr_zifencei_multilib_b_ext-win\bin\riscv32-unknown-elf-gdb.exe" ^
  firmware\application_5301\build_dfu\output\akaLinkPro_App.elf
```

```
(gdb) target extended-remote localhost:2331
(gdb) monitor reset
(gdb) load
(gdb) monitor reset
(gdb) break main
(gdb) continue
```

---

## 7. 验证设备状态

APP 运行时的 USB 描述符：`VID_0D28 & PID_0204`，接口 MI_00=CMSIS-DAP、MI_01=CDC(COM)、MI_03=HID、MI_04=WebUSB、MI_05=DFU Runtime。
Bootloader DFU 模式：`VID_0D28 & PID_0205`（仅 DFU）。

```powershell
Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -match 'VID_0D28' } |
  Select-Object Status,Class,FriendlyName,InstanceId | Format-Table -AutoSize
```

读回 flash（确认已写入）：

```
device HPM5301xEGx
si JTAG
jtagconf -1 -1
speed 4000
connect
mem8 0x80020000 4      // 应为 21 4D 50 48  ("HPM!")
mem32 0x80000000 4
Exit
```

---

## 8. 5V 供电与 UART2 ↔ CDC 串口桥

### 5V 输出
- **PB13**（`POWER_5V_EN_PIN`）默认配置为 GPIO 输出**高电平**，给电平转换供电。
- 实现在 `boards/akaLinkPro/pinmux.c` 的 `init_power_pins()`（由 `board_init_gpio_pins()` 调用），
  可用 `board_set_5v_output(0/1)` 关闭/开启。

### UART2 复用规则（关键）
PA08 = JTDI/UART2_TXD，PA09 = JTDO/UART2_RXD，同一对引脚在两种功能间切换：

| DAP 状态 | PA08/PA09 | CDC COM 口 |
| --- | --- | --- |
| 未连接 / 空闲 / SWD 模式 | UART2 | 可用 |
| JTAG 模式 | FGPIO (TDI/TDO) | 保留但无数据 |

- 挂接点在 `src/dap/DAP_config.h`：`PORT_SWD_SETUP()`/`PORT_OFF()` 调 `uartx_enter_com_mode()`，
  `PORT_JTAG_SETUP()` 调 `uartx_enter_jtag_mode()`。
- `uartx_enter_jtag_mode()` **必须把 PA08/PA09 的 `FUNC_CTL` 清 0**，否则 UART2 仍占用引脚、JTAG 失效。
- 数据通路：USB CDC OUT → `g_usbrx` → UART2 TX（DMA）；UART2 RX（DMA）→ `g_uartrx` → CDC IN。
- 波特率由主机 `SET_LINE_CODING` 决定。
- **UART2 时钟（默认 80MHz，符合手册）**：`PLL0CLK0(720M)/9 = 80MHz`，
  硬件/软件上限 `uart_clk/8 = 10Mbps`；`UART2_CLK_DIV=9`、`UART2_MAX_BAUDRATE=10000000`。
- `UART2_OVERCLOCK=1` 时改用 `720/4 = 180MHz`（**超出手册限制，不保证所有芯片稳定**），
  上限 22.5Mbps，且 11.25/15/18Mbps 等可精确生成。
  通过 `CMakeLists.txt` 里 `sdk_compile_definitions(-DUART2_OVERCLOCK=1)` 打开。
- 无法精确生成的按就近取整（`uart2_round_baudrate()`）；实际值存于 `g_uart2_applied_baud`。

### cdc_interface.c 重要实现点（历史坑）
- `uartx_preinit()` 必须在 `chry_dap_init()` **之后**调用（`DAP_SETUP()` 会把 PA08/09 设为 GPIO），见 `src/main.c`。
- 必须先 `dma_mgr_init()`，否则 `dma_mgr_request_resource()` 全部失败、TX/RX DMA 不工作。
- RX 采用 **DMAV2 infinite-loop 圆形缓冲**（`en_infiniteloop=true`，`UART_RX_DMA_BUFFER_SIZE=8192`）：
  硬件自动回卷，软件不做 disable/restart。
  - 写入位置用 DMA 的 live `CHCTRL.DSTADDR - buf_base`；小于读位置时按“尾部 + 头部”两段 flush。
  - `en_infiniteloop` 要求 `linked_ptr == 0`，仅 DMAV2 支持。
- **定时器驱动 flush（关键）**：SWD 的延迟采样会在临界区里关总中断，IDLE/满缓冲中断会被抖动。
  因此用 `GPTMR0` 的 **reload 中断** 周期触发 `uart_flush_timer_isr()`，把 DMA 缓冲里的数据搬进
  `g_uartrx`，不再依赖 UART IDLE 和 buffer-full 两个中断。
  - 周期**按当前波特率动态设置**：目标每次 flush 约 `UART_FLUSH_TARGET_BYTES`(1024) 字节，
    `interval_us = 1024*10*1e6/baud`，限制在 `[200us, 10ms]`；低波特率不会过度打扰 CPU。
    `uart_flush_timer_set_baud()` 在 `SET_LINE_CODING` 后按实际波特率调用，
    用 `gptmr_channel_config_update_reload()` 只改 RLD（reload 中断周期随之改变）。
    实测：9600→10ms，9M→1.14ms，22.5M→455us。
  - RX DMA 缓冲 `UART_RX_DMA_BUFFER_SIZE=16384`（≥ 2 个 flush 周期数据 + 余量）。
  - `g_uartrx` 放大到 32KB 以吸收主循环被 SWD 阻塞的时间。
- **`g_uartrx` 所有读写必须在临界区**：生产者是 DMA TC / 定时器 / IDLE / 主循环轮询，
  消费者是 USB IN 完成回调和主循环；`chry_ringbuffer` 非线程安全。
  历史上漏掉 DMA TC 回调的临界区会导致 ring 索引错乱、CDC 多发字节（掉/重数据）。
- `uartx_rx_dma_restart()` 仅在波特率改变时清空 ringbuffer 并重启 RX。
- `PORT_SWD_SETUP()` **不要**再配置 TDI/TDO（PA08/09）：SWD 不用它们，反复 `DAP_Connect`
  重新配置会把 UART2 引脚打断造成丢字节。由 `uartx_enter_com_mode()` 负责保持为 UART2。
- DTR/RTS 默认不驱动（`UART2_DRIVE_DTR_RTS=0`），本板无对应网络。

### 波特率钳制与就近取整
- 软件上限 `UART2_MAX_BAUDRATE`：默认 `10000000`(10M)，`UART2_OVERCLOCK=1` 时 `22500000`；
  主机请求超过上限一律钳到上限。
- `uart2_round_baudrate()` 在 `osc∈{8..30 偶数}`、`div∈[1,0xFFFF]` 中取
  `|uart_clk/(div*osc) - 目标|` 最小的可达波特率（不套用 SDK 的 3% 容差），
  无法整除的速率会落到最近的可达值。
- 实际写入的波特率存于全局 `g_uart2_applied_baud`（可用 J-Link/GDB 读取核对）。

### 串口速率上限与回环测试
- 硬件公式：`baud = uart_clk / (div * osc)`，`osc` 为 8~30 偶数；硬件上限 = `uart_clk / 8`。
- 默认 UART2 时钟 = `PLL0CLK0(720MHz)/9 = 80MHz`（手册上限）→ 上限 **10 Mbps**；
  `UART2_OVERCLOCK=1` → `720/4 = 180MHz` → 上限 **22.5 Mbps**（超规格）。
- 实测（默认 80MHz，详见 `script_test/uart_loopback_common.py`）：9600~10M 全部 OK；
  10M 线速效率 ~99.6%；>10M 的请求被钳到 10M。
- 回归脚本（PA08/PA09 短接）：
  - `script_test/uart_loopback_common.py`：常见速率 + 请求/实际波特率/误差/吞吐 详细表。
  - `script_test/uart_loopback.py` / `uart_loopback_hs.py`：低/高速压力回环。
  - `script_test/test_modeswitch.py`：JTAG/SWD/空闲 引脚复用切换。

### CDC + SWD 同时满载测试
用 `script_test/swd/run_benchmark.py <kHz> <rounds>`（基于 `benchmark_readback.tcl`）做 SWD 读写校验，
同时用 `script_test/uart_stress_stream.py COM75 9000000 <秒>` 连续灌 CDC 数据做回环比对。
SWD 目标可为 STM32F1 等；本板 SWD 实跑 20/36/45/60MHz。

实测结论（SWD 1000 轮 + CDC 9Mbps 连续流，全部 OK，无掉数据）：
- 20MHz / 36MHz / 45MHz / 60MHz SWD 均 1000/1000 PASS。
- CDC 每轮约 7~10MB 连续回环，`rx==sent`、无重复/丢失。
- SWD 写/读吞吐：20M≈1.5/1.4，45M≈2.8/2.3，60M≈3.4/2.8 MiB/s（取决于目标）。
- 要突破 9M 上限需同时调大 `UART2_MAX_BAUDRATE` 并提高 UART 时钟
  （如 720/4=180MHz），且必须确认不超过 UART 外设数据手册的输入时钟上限（当前 SDK 未给出）。

---

## 9. 常见坑（务必遵守）

1. **只支持 JTAG**：本 J-Link 走 SWD 无法连接，且 JTAG 必须 `jtagconf -1 -1` 自动探测，否则停在交互提示。
2. **不要用 `erase` 全片擦除**：J-Link 对本板 QSPI 报 "Only internal flash banks will be erased"，
   `exec EnableEraseAllFlashBanks` + `erase` 会**卡住**。只用 `loadfile`，它会自动擦写到的扇区。
3. **必须加 `-NoGui 1 -ExitOnError 1`**：否则可能弹窗阻塞脚本；脚本内用 `Sleep` 加延时。
4. **APP 地址固定 `0x80020000`**：靠 bootloader 跳转；不要用 flash_xip 放到 `0x80000000`。
5. `.vscode/` 被 `.gitignore` 忽略，配置改了不会进 git。
6. `application_5301/CMakeLists.txt` 已把 `_dfu_bl_length=0x20000` 限定在非 `flash_xip` 构建，
   并新增 `.hex` 生成；改链接脚本时注意别破坏这两点。

---

## 10. 修改代码时的自检

1. `firmware\application_5301\build_dfu.bat` 能过。
2. `firmware\bootloader_dfu\build_xip.bat` 能过。
3. J-Link 烧录后设备枚举正常（`VID_0D28`）。
4. VSCode F5 能在 `main` 命中断点。
5. 串口回环（RXD-TXD 短接）在 SWD/空闲下能通过，JTAG 下无回显。
6. 不要动 `build.bat` / `program.bat` / `build_zcc.bat` 的既有行为。
