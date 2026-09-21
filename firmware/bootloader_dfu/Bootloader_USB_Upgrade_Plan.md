# Bootloader U 盘升级（MSC + 虚拟 FAT）方案

> 参考：`X:\cproject\Debugger\akaLinkCompact\firmware\Bootloader`（CH32V305）
> 目标：为 HPM5301 bootloader 增加 MSC 枚举，U 盘内含 `INFO.TXT` 与网页快捷方式，支持拖拽升级。
> 状态：**已实施并验证通过**

## 0. 实施结果

- USB：**复合 DFU + MSC**，最终 PID **`0x0207`**（0x0205 为旧 DFU-only；0x0206 在开发期被 Windows
  缓存了旧 OS 描述符）。MI_00=DFU（WinUSB，经 MS OS 2.0/WCID 自动安装）、MI_01=MSC。
- U 盘：128MB FAT16（卷标 `AKALINKPRO`），`INFO.TXT` / `AKALINK.URL` / `AKALINK.HTM`。
- 拖拽升级：任意 `*.BIN` → 按 FAT 链写入 APP 区（跳过 `System Volume Information`）→ 延时 1s 复位 → 校验跳转。
- `dfu-util -l` / `-D _pack.bin` 均可用。
- 虚拟 FAT：标准 FAT16 512B 扇区，64 sectors/cluster，RAM 元数据（boot/FAT/root），数据簇映射 APP flash；
  写上限 `APP_MAX = 0x800FE000 - 0x80020000`，**不触碰尾部 EasyFlash/保留区**。
- 需成对升级：Bootloader 与 APP 的头格式/入口 `0x80020100`。

---

## 1. 参考工程做法（CH32V305）

- Bootloader **MSC-only**（VID 0D28 / PID 0204），枚举成一个 64×2048B 的虚拟 FAT16 盘。
- `vfat/flash_fat16.c`：静态数组提供 boot sector / FAT / root dir / 一个文本文件；
  `usbd_msc_sector_read/write` 把逻辑块映射到这些数组与 flash。
- root 目录写入时识别 `*.BIN` 文件名与 `file_size`；后续数据写入按偏移写到 APP flash
  （`VFAT16_FLASH_START_ADDR`），写满后置 `g_app_download_finished=1`，主循环延时 2s 复位。
- `fat16_file_init()` 填充 info 文本：芯片 UID、HWVER、BLVER、FWVER、CRC 状态。
- **不足**：没有网页快捷方式；FAT/FAT 写入被忽略（一致性弱）；仅 MSC，无 DFU。

---

## 2. 拟采用的 HPM5301 设计

### 2.1 USB 拓扑（需确认，见 §5-Q1）

推荐 **复合设备：DFU + MSC**，保持现有 dfu-util 路径可用，同时出现 U 盘：
- 接口 0：DFU（`usbd_dfu_init_intf`，仅 EP0）—— 保留。
- 接口 1：MSC（`usbd_msc_init_intf`，2 个 bulk EP，如 `0x81`/`0x02`）。
- 描述符参考 `application_5301/src/usb/usb_composite.c` 的 composite 写法（含 `MSC_DESCRIPTOR_INIT`）。
- VID/PID 保持 bootloader 的 `0D28:0205`；Product 改为 `akaLinkPro U-Disk` / `akaLinkPro DFU`。

### 2.2 虚拟文件系统（RAM 元数据 + flash 数据区）

采用**标准 FAT16、512B 扇区**，元数据放在 RAM 缓冲区，数据簇直接映射 APP flash：

| 区域 | 位置 | 说明 |
| --- | --- | --- |
| Boot 扇区 | RAM，LBA 0 | 标准 BPB（512B/sector, 8 sec/cluster, 2 FAT, 512 root entries, 卷 16MB 以便判定为 FAT16） |
| FAT1/FAT2 | RAM | 初始化时写入卷标 + 两个文件 + 升级文件的簇链 |
| 根目录 | RAM | `INFO.TXT`、`AKALINK.URL`（+ 主机新建的升级文件） |
| 数据簇 0.. | 文件内容 | `INFO.TXT`/`URL` 由 RAM 生成；升级文件簇 → 直接读写 APP flash |

- 卷大小：报告 16MB（512B×32768），保证主机按 FAT16 挂载；未使用扇区读返回 0、写忽略。
- **`INFO.TXT`** 内容（示例）：
  ```
  SN:XXXXXXXXXXXX
  HWVER:A.0
  BLVER:1.0
  FWVER:0.1
  DESC:akaLinkPro CMSIS-DAP
  CRC32:pass|fail
  Drop a .bin firmware here to upgrade.
  ```
  数据来源：OTP UID（`drv_read_uid`/`otp`）、boot info 块 `0x8001F000`、APP 头 `0x80020000`（CRC 校验状态）。
- **网页快捷方式**：`AKALINK.URL`（Windows Internet Shortcut）
  ```
  [InternetShortcut]\r\nURL=https://akkako.github.io/akaLinkPro/\r\n
  ```
  （如需跨平台可另加 `AKALINK.HTM` 做 meta 跳转，见 §5-Q3。）

### 2.3 U 盘升级流程

1. 主机把 `akaLinkPro_App_pack.bin` 拖入 U 盘 → 在根目录新建一个 `*.BIN`。
2. 根目录写入回调解析新条目，记录 `start_cluster`/`file_size`，并把该文件数据区映射到
   `APP_BASE(0x80020000)` 起；数据写入经 `dfu_flash_port`/ROM API 擦写 APP 区。
3. 文件写满 → 置 `g_app_download_finished`；主循环延时 ~2s（等主机 flush）后复位。
4. Bootloader 复位后按 `app_image_valid()` 校验（签名+长度+CRC32）→ 通过则跳转，失败停留并再次提供 U 盘。

> 升级镜像必须是**打包镜像**（含 256B 头），与 J-Link/dfu-util 一致。

### 2.4 LED 提示（可选）

- 进入 Bootloader：双灯慢闪（现状）。
- U 盘挂载/可写：可约定某种闪法。
- 升级写入中/完成：可约定（见 §5-Q4）。

---

## 3. 涉及文件

| 文件 | 改动 |
| --- | --- |
| `bootloader_dfu/src/usb_config.h` | 打开 `CONFIG_USB_DEVICE_MSC` |
| `bootloader_dfu/CMakeLists.txt` | 加入 `src/vfat.c`、`src/msc_if.c`（MSC 类由 SDK 按 `CONFIG_USB_DEVICE_MSC` 编译） |
| `bootloader_dfu/src/dfu_desc.c` | 改为 DFU+MSC 复合描述符；注册两个接口 |
| `bootloader_dfu/src/main.c` | 初始化 DFU+MSC；主循环处理升级完成复位 |
| `bootloader_dfu/src/vfat.c/.h` | 新增：RAM FAT16 元数据 + `INFO.TXT`/`URL` 生成 + 数据簇→APP flash 映射 |
| `bootloader_dfu/src/msc_if.c` | 新增：`usbd_msc_get_cap/sector_read/sector_write` |
| `bootloader_dfu/src/boot_port_board_hpm.c` | 读取 OTP UID（序列号）如需 |
| 文档 | 新增本文件；更新 `SKILL.md`（bootloader 变复合 DFU+MSC、U 盘说明） |

---

## 4. 风险 / 注意

1. 改动 USB 描述符有**破坏现有 dfu-util 升级**的风险，需两种方式都回归。
2. 主机对 FAT16 的一致性要求：必须正确维护 FAT/根目录/簇链；否则 Windows 可能报“需要格式化”。
3. 升级写入与 DFU 写入共用 APP 区，需保证互斥（同一时刻只走一种）。
4. 升级中掉电：APP 头/CRC 不完整 → Bootloader 校验失败，停留 U 盘，可重试。
5. 写 flash 时中断/耗时（扇区擦除）与 MSC 事务的时序。
6. 大文件（107KB ≈ 214×512B 扇区）需要正确的簇链与顺序写入。

---

## 5. 待确认

- **Q1 USB 拓扑**：复合 **DFU+MSC**（推荐，保留 dfu-util）？还是 MSC-only（替换 DFU）？
- **Q2 虚拟 FAT 规格**：标准 FAT16/512B（推荐）/ 参考工程的 FAT16/2048B / FAT12 软盘镜像？
- **Q3 快捷方式文件**：仅 `AKALINK.URL`（Windows）？还是再附一个 `AKALINK.HTM`（跨平台 meta 跳转）？
- **Q4 升级完成行为**：写满即延时 2s 复位（参考做法，推荐）？是否需要 LED 指示/更长时间？
- **Q5 升级文件名**：接受任意 `*.BIN`（参考做法）？还是仅接受固定名 `FIRMWARE.BIN`（更可控）？

确认后按 §3 实施，并用 Windows 挂载 → 读取文件 → 拖拽升级 全流程回归。
