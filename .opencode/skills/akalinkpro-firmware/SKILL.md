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

## 0. 其他 skill 索引

除了阅读本 skill 外，还应该阅读以下skill：

```
	jlink/SKILL.md
```

## 1. 目录结构

```
firmware/
  version.json                      版本/描述/硬件信息唯一来源（pack 读取）
  tools/pack.py                     构建后注入 APP 头(长度/CRC32/版本/时间/描述) 与 BL 信息块
  application_5301/                 主固件 (CMSIS-DAP)
    CMakeLists.txt                  自定义链接脚本 linker/flash_dfu_app.ld；POST_BUILD 调用 pack.py
    linker/flash_dfu_app.ld         预留 256B APP 头，入口 0x80020100（CUSTOM_GCC_LINKER_FILE）
    build_dfu.bat                   构建到 ./build_dfu（不调用 dfu-util）
    program.bat                     对 build_dfu 产物执行 dfu-util 烧录（用 _pack.bin）
    flash_jlink.bat                 build_dfu + JLink 烧录 APP（用 _pack.hex）
    gdb_server.bat                  启动 JLinkGDBServerCL（VSCode 调试用）
    Flash_Memory_Map.md             Flash 布局（含元数据/ EasyFlash）
    Firmware_Integrity_Plan.md      完整性校验+版本嵌入方案与自检
    Custom HID Protocol.md          HID 协议（配置/元数据）
    boards/akaLinkPro/              board.c/h, clock.c/h, pinmux.c/h, akaLinkPro.yaml
    src/  main.c, usb/, dap/, api/, led/, drv/, easyflash/, dfu/
  bootloader_dfu/                   DFU Bootloader
    CMakeLists.txt                  链接 flash_xip.ld，128K flash；POST_BUILD 调用 pack.py
    build.bat / build_xip.bat       ./build / ./build_xip（JLink 流程）
    flash_jlink.bat                 build_xip + JLink 烧录 bootloader（用 _pack.hex）
    src/  main.c, dfu_desc.c, hpm_dfu_trigger.c, boot_port_board_hpm.c, dfu_flash_port.c,
          vfat.c/.h（虚拟 FAT16 U 盘）, msc_if.c（MSC 回调）
    Bootloader_USB_Upgrade_Plan.md  U 盘升级方案
```

构建产物目录 `build` / `build_xip` / `build_dfu` 均已在 `.gitignore` 中忽略。
每次构建后生成 **打包镜像** `*_pack.hex` / `*_pack.bin`（含元数据），烧录请用打包镜像。

---

## 2. 内存布局与关键地址

HPM5301，外挂 1MB QSPI NOR，XIP 基址 `0x80000000`。

| 区域 | 地址 | 链接脚本 | 说明 |
| --- | --- | --- | --- |
| Bootloader 代码 | `0x80000000 - 0x8001EFFF` | `flash_xip.ld` | 含 `nor_cfg_option`@0x400、`boot_header`@0x1000、`.start`@0x3000 |
| Bootloader 信息块 | `0x8001F000 - 0x8001F0FF` (256B) | — | `pack.py` 写入：BL 版本/编译时间/硬件版本/生产日期 |
| APP 头 | `0x80020000 - 0x800200FF` (256B) | `flash_dfu_app.ld` | 签名 + 长度 + CRC32 + 版本/编译时间/描述 |
| APP 代码 | `0x80020100 - 0x800FDFFF` | `flash_dfu_app.ld` | **入口 `0x80020100`** |
| EasyFlash ENV | `0x800FE000 - 0x800FFFFF` (8K) | — | 配置持久化（2×4K 扇区，EasyFlash） |
| ILM | `0x00000000` (128K) | — | 向量表 / `.fast` |
| DLM | `0x00080300` | — | data / bss / heap / stack |
| AHB_SRAM | `0xF0400000` (32K) | — | `.ahb_sram` |

- DFU 签名：`0x80020000` 必须为 `0x48504D21`（`"HPM!"`，`BOARD_DFU_SIGNATURE`）。
- Bootloader 在 `hpm_dfu_check_bootloader_request()` 中校验 **签名 + 长度 + CRC32**（`app_image_valid()`），
  成功跳到 `0x80020100`，**失败则停留 DFU 模式**。
- **APP 用自定义链接脚本** `application_5301/linker/flash_dfu_app.ld`（经 `CUSTOM_GCC_LINKER_FILE`），
  在 `.dfu_signature` 后 `. = ALIGN(0x100)` 预留 APP 头；APP 不再直接链接到 `flash_dfu.ld`。
- 头/信息块由 `firmware/tools/pack.py` 注入，版本源 `firmware/version.json`；编译时间取打包时刻。
- 详见 `application_5301/Flash_Memory_Map.md`、`application_5301/Firmware_Integrity_Plan.md`。
- DFU 引导程序自身只能 J-Link 烧录（不升级自身）。

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

:: App (flash_dfu @0x80020000) -> build_dfu\   (生成 .hex/.bin 与打包 img)
firmware\application_5301\build_dfu.bat
```

构建后处理：两个工程都会在链接后调用 `firmware/tools/pack.py` 生成**打包镜像**：
- App：`build_dfu\output\akaLinkPro_App_pack.bin`（dfu-util）/ `_pack.hex`（J-Link）
- Boot：`build_xip\output\akaLinkPro_Boot_pack.hex`（含 `0x8001F000` 信息块）

其它入口：
- `bootloader_dfu\build.bat`（`./build`）
- `application_5301\build_dfu.bat` + `program.bat`（构建后用 dfu-util 烧录 `_pack.bin`）

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
脚本内 `loadfile` 指向**打包镜像**（`*_pack.hex`）：APP 头 / Bootloader 信息块已包含其中；
烧未打包的 `.hex` 会因缺少元数据导致 Bootloader 校验失败而停留在 DFU。

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

### 5.2 DFU 烧录

```bat
firmware\application_5301\build_dfu.bat   :: 先构建到 build_dfu\
firmware\application_5301\program.bat     :: 再 dfu-util 下载
:: program.bat 等价于:
:: dfu-util -a 0 -E 1 -s 0x80020000:leave -D build_dfu\output\akaLinkPro_App_pack.bin
```

APP 自带 DFU runtime 接口，`dfu-util` 会先发 `DFU_DETACH` 触发重启进入 bootloader，再传输。
**必须用 `_pack.bin`**（含 256B APP 头 + CRC32）；传输完成后 Bootloader 会校验并跳转。
Bootloader 自身不能用 DFU 升级（仅 J-Link）。

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
Bootloader 模式：`VID_0D28 & PID_0207`（**复合设备 DFU + MSC**），MI_00=DFU（WinUSB，支持 dfu-util）、MI_01=MSC（U 盘）。
- 说明：0x0205 是旧 DFU-only PID，0x0206 在开发期被 Windows 缓存了旧 OS 描述符，故最终复合 PID 用 **0x0207**。
- WinUSB 由 **MS OS 2.0 描述符（WCID）** 自动安装到 DFU 接口，`dfu-util` 无需再手动绑驱动。

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
mem8 0x80020000 64     // APP 头：21 4D 50 48 + 长度 + CRC32 + 版本 + 时间 + 描述
mem8 0x80020100 8      // APP 入口代码（_start）
mem8 0x8001F000 64     // Bootloader 信息块："BLI1" + BL 版本 + 时间 + 硬件版本 + 生产日期
mem8 0x800FE000 16     // EasyFlash 扇区头，含 "EF40" (45 46 34 30)
Exit
```

---

## 7.1 Bootloader U 盘升级（MSC + 虚拟 FAT）

Bootloader 进入 DFU/升级模式时同时枚举一个 **MSC U 盘**（128MB FAT16，卷标 `AKALINKPRO`），
由 `bootloader_dfu/src/vfat.c` 提供（RAM 元数据 + APP flash 数据区，实际可写上限 = APP 区 `0x80020000..0x800FE000`，**不会碰尾部 EasyFlash**）：

| 文件 | 内容 |
| --- | --- |
| `INFO.TXT` | SN(OTP UID) / HWVER / BLVER / FWVER / DESC / CRC 状态 |
| `AKALINK.URL` | Internet Shortcut → `https://akkako.github.io/akaLinkPro/` |
| `AKALINK.HTM` | 同 URL 的 HTML meta 跳转（跨平台） |

- **拖拽升级**：把打包镜像改名（任意 `*.BIN`，如 `FIRMWARE.BIN`）拖入 U 盘 → 数据按 FAT 链写入
  APP 区 → 写满后延时 1s 复位 → Bootloader 校验（签名+长度+CRC32）通过则运行 APP，失败继续留在 U 盘。
- **镜像必须是打包镜像** `akaLinkPro_App_pack.bin`（含 256B 头），与 J-Link/dfu-util 一致。
- 只写属于该 `*.BIN` 的簇（会跳过 Windows 自动创建的 `System Volume Information`）。
- 进入升级模式：APP 的 `CMD_ENTER_DFU(0xFF)`、按住 boot 键、或 APP 校验失败时自动停留。
- 相关文档：`bootloader_dfu/Bootloader_USB_Upgrade_Plan.md`。

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
4. **必须用打包镜像烧录**（`*_pack.hex` / `*_pack.bin`）：App 头/BL 信息块由 `pack.py` 注入，
   直接烧未打包镜像会被 Bootloader CRC 校验拒绝并停留 DFU。
5. **APP 基址 `0x80020000`、入口 `0x80020100`**：靠 bootloader 跳转；不要用 flash_xip 放到 `0x80000000`；
   改入口/头需同步改 bootloader 的 `app_image_valid()` 与 `hpm_dfu_jump_to_app()`。
6. `.vscode/` 被 `.gitignore` 忽略，配置改了不会进 git。
7. `application_5301/CMakeLists.txt`：`_dfu_bl_length=0x20000` + `_flash_size=0xFE000`（尾部 8K EasyFlash）
   仅用于非 `flash_xip`；并用 `CUSTOM_GCC_LINKER_FILE=linker/flash_dfu_app.ld`。改链接脚本时别破坏这些。
8. **DFU 保护**：Bootloader 的 `dfu_flash_port.c` 把擦写限制在 `< 0x800FE000`，DfuSe 描述也扣除了尾部 8K；
   不要用整片全量 DFU 覆盖 EasyFlash 区（Bootloader 也会拒绝越界擦写）。

---

## 10. 修改代码时的自检

1. `firmware\application_5301\build_dfu.bat` 与 `firmware\bootloader_dfu\build_xip.bat` 能过，
   且输出 `[pack app]` / `[pack boot]`（含 ver/len/crc/time）。
2. 烧录**打包镜像**后设备枚举正常（APP：`VID_0D28 PID_0204`；校验失败/升级模式：`PID_0207` 复合 DFU+MSC）。
3. 破坏 APP 头后复位应停留 Bootloader（完整性保护生效）。
4. VSCode F5 能在 `main` 命中断点。
5. 串口回环（RXD-TXD 短接）在 SWD/空闲下能通过，JTAG 下无回显。
6. HID `0x12`–`0x17` 返回 `version.json` 中配置的版本/时间。
7. APP 已移除 `build.bat` / `build_zcc.bat`：统一用 `build_dfu.bat` 构建，`program.bat`（dfu-util）或 `flash_jlink.bat`（J-Link）烧录；均使用 `_pack` 产物。
