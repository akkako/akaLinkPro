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

## 8. 常见坑（务必遵守）

1. **只支持 JTAG**：本 J-Link 走 SWD 无法连接，且 JTAG 必须 `jtagconf -1 -1` 自动探测，否则停在交互提示。
2. **不要用 `erase` 全片擦除**：J-Link 对本板 QSPI 报 "Only internal flash banks will be erased"，
   `exec EnableEraseAllFlashBanks` + `erase` 会**卡住**。只用 `loadfile`，它会自动擦写到的扇区。
3. **必须加 `-NoGui 1 -ExitOnError 1`**：否则可能弹窗阻塞脚本；脚本内用 `Sleep` 加延时。
4. **APP 地址固定 `0x80020000`**：靠 bootloader 跳转；不要用 flash_xip 放到 `0x80000000`。
5. `.vscode/` 被 `.gitignore` 忽略，配置改了不会进 git。
6. `application_5301/CMakeLists.txt` 已把 `_dfu_bl_length=0x20000` 限定在非 `flash_xip` 构建，
   并新增 `.hex` 生成；改链接脚本时注意别破坏这两点。

---

## 9. 修改代码时的自检

1. `firmware\application_5301\build_dfu.bat` 能过。
2. `firmware\bootloader_dfu\build_xip.bat` 能过。
3. J-Link 烧录后设备枚举正常（`VID_0D28`）。
4. VSCode F5 能在 `main` 命中断点。
5. 不要动 `build.bat` / `program.bat` / `build_zcc.bat` 的既有行为。
