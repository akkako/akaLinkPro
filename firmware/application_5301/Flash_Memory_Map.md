# akaLink Pro Flash 存储区分布

适用芯片：**HPM5301**，外挂 **1 MB QSPI NOR**，XIP 基址 `0x80000000`。
NOR 扇区（sector）大小：**4 KB (0x1000)**，共 256 个扇区（扇区号 0 – 255）。

> 所有地址均为 CPU 视角的 flash 映射地址（XIP）。写入/擦除通过 ROM API
> (`rom_xpi_nor_*`) 完成，偏移量为「映射地址 − 0x80000000」。

## 1. 总体分布

| 区域 | 起始地址 | 结束地址 | 大小 | 扇区号 | 说明 |
| --- | --- | --- | --- | --- | --- |
| Bootloader 代码 | `0x80000000` | `0x8001EFFF` | < 124 KB | 0 – 30 | DFU 引导程序（实际 ~40 KB） |
| Bootloader 信息块 | `0x8001F000` | `0x8001F0FF` | 256 B | 31 | 版本/编译时间/硬件版本/生产日期 |
| APP 头 | `0x80020000` | `0x800200FF` | 256 B | 32 | DFU 签名 + 长度 + CRC32 + 版本/时间/描述 |
| APP 代码 | `0x80020100` | `0x800FDFFF` | ~895 KB | 32 – 253 | 应用程序链接区，入口 `0x80020100` |
| EasyFlash ENV 区 | `0x800FE000` | `0x800FFFFF` | 8 KB | 254 – 255 | 配置持久化（见 §4） |

- APP 头首 4 字节为 DFU 签名 `0x48504D21`（"HPM!"），代码入口 `0x80020100`。
- Bootloader 校验通过后跳转到 `0x80020100`。
- 元数据由构建后 `firmware/tools/pack.py` 注入，详见 §3.2 / §4.5。

## 2. Bootloader 区（`0x80000000` – `0x8001FFFF`）

| 地址 | 内容 | 说明 |
| --- | --- | --- |
| `0x80000400` | `nor_cfg_option` | QSPI NOR 配置字（Rom 引导用） |
| `0x80001000` | `boot_header` | HPM 启动头 |
| `0x80003000` | `.start` / 代码 | Bootloader 可执行代码 |
| `0x8001FFFC` | 保留 | 区尾，下一个区域为 APP |

## 3. APP 固件区（`0x80020000` – `0x800FDFFF`）

| 地址 | 内容 | 说明 |
| --- | --- | --- |
| `0x80020000`–`0x800200FF` | **APP 头** | 见 §3.1；由 `pack.py` 注入 |
| `0x80020100` | 代码入口 `_start` | 实际占用约 80 KB（随版本变化） |
| 区尾 `0x800FDFFF` | APP 区上界（含） | 其后为 8 KB EasyFlash 区 |

APP 区使用**自定义链接脚本** `linker/flash_dfu_app.ld`（经 `CUSTOM_GCC_LINKER_FILE` 指定），
在 `.dfu_signature` 之后 `. = ALIGN(0x100)` 预留 APP 头，使入口落到 `0x80020100`。
区大小仍为 `APP_LENGTH = _flash_size − _dfu_bl_length = 0xFE000 − 0x20000 = 0xDE000`。

### 3.1 APP 头结构（256 B @ `0x80020000`）

| 偏移 | 长度 | 字段 | 来源 |
| --- | --- | --- | --- |
| `0x00` | 4 | DFU 签名 `0x48504D21` | 链接器（保持） |
| `0x04` | 4 | 代码长度 `app_code_len` | `pack.py` |
| `0x08` | 4 | 代码 CRC32（种子 `0x0D000721`） | `pack.py` |
| `0x0C` | 4 | 头格式版本 = 1 | `pack.py` |
| `0x10` | 8 | 固件版本串 | `version.json: app_fw_ver` |
| `0x18` | 20 | 固件编译时间 `YYYY/MM/DD HH:MM:SS` | 打包时刻 |
| `0x2C` | 24 | 固件描述 | `version.json: app_desc` |
| `0x44` | 0xBC | 保留 | — |

## 3.2 Bootloader 信息块（256 B @ `0x8001F000`）

| 偏移 | 长度 | 字段 | 来源 |
| --- | --- | --- | --- |
| `0x00` | 4 | magic `"BLI1"` (`0x31494C42`) | `pack.py` |
| `0x04` | 4 | Bootloader 代码长度 | `pack.py` |
| `0x08` | 4 | Bootloader CRC32（预留，未校验） | — |
| `0x0C` | 4 | 格式版本 = 1 | `pack.py` |
| `0x10` | 8 | Bootloader 版本串 | `version.json: bl_ver` |
| `0x18` | 20 | Bootloader 编译时间 | 打包时刻 |
| `0x2C` | 8 | 硬件版本串 | `version.json: hw_ver` |
| `0x34` | 20 | 硬件生产日期 | `version.json: hw_prod_date` |

### 3.3 版本信息源

统一配置于 `firmware/version.json`（`app_fw_ver` / `app_desc` / `bl_ver` / `hw_ver` / `hw_prod_date`），
由 app 与 bootloader 的 `pack.py`（`firmware/tools/pack.py`）在构建后写入；编译时间取打包时刻。

## 4. 参数存储区（`0x800FE000` – `0x800FFFFF`，EasyFlash）

由 **EasyFlash（ENV, NG 模式）** 管理的持久化区，占用 2 个 4 KB 扇区（8 KB），
自带磨损平衡与掉电保护，不再使用早期的双槽 ping-pong 自定义格式。

| 区域 | 地址范围 | 扇区号 | 大小 | 说明 |
| --- | --- | --- | --- | --- |
| EasyFlash ENV 区 | `0x800FE000` – `0x800FFFFF` | 254 – 255 | 8 KB | `ENV_AREA_SIZE`，`EF_START_ADDR` |

- 逻辑：ENV key `"cfg"` 保存整个 `api_param_t` blob（`ef_get_env_blob`/`ef_set_env_blob`）。
- 物理：每个扇区头 magic `EF40`；每个 ENV 节点头 magic `KV40` + 长度 + CRC32 + 状态位；
  2 个扇区间自动 GC / 磨损平衡，一个扇区通常留作 GC 空扇区。
- 出厂默认：由 `api_param.c` 的 `g_param_default` 提供（`ef_port.c` 的 `default_env_set`），
  首次上电 / 扇区无效 / `EF_ENV_VER_NUM` 变化时写入。
  默认值：`output_mode=0, usb5v=1, clock_accel=0, led1=4, led2=5, vref=3300`。

分层实现：

| 层 | 文件 | 职责 |
| --- | --- | --- |
| 硬件驱动 (`drv`) | `src/drv/drv_flash.c/.h` | 封装 ROM API：`drv_flash_init/read/erase/write`，处理字节对齐与 Cache |
| 存储库端口 | `src/easyflash/port/ef_port.c` | 把 EasyFlash 的 `ef_port_read/erase/write/env_lock` 及默认 ENV 接到 `drv_flash` |
| 存储库 | `src/easyflash/src/*` | EasyFlash 本体（ENV/GC/CRC） |
| 应用 | `src/api/api_param.c` | 通过 `ef_get_env_blob`/`ef_set_env_blob("cfg")` 存取配置 |

> 驱动/存储库不感知业务字段，应用层只面对 key/value，换存储介质只需改 `drv` 层。

## 5. 相关代码 / 链接约束（改尾部保留区时必须同步）

| 位置 | 约束 | 当前值 |
| --- | --- | --- |
| `application_5301/CMakeLists.txt` | APP 链接 `_flash_size`（决定 APP 区上界） | `0xFE000`（= 1 MB − 8 KB） |
| `application_5301/CMakeLists.txt` | Bootloader 预留 `_dfu_bl_length` | `0x20000` |
| `application_5301/CMakeLists.txt` | APP 自定义链接脚本 `CUSTOM_GCC_LINKER_FILE` | `linker/flash_dfu_app.ld` |
| `application_5301/CMakeLists.txt` | APP POST_BUILD 打包（写入 APP 头） | `tools/pack.py app` |
| `bootloader_dfu/CMakeLists.txt` | Bootloader POST_BUILD 打包（写入信息块） | `tools/pack.py boot` |
| `firmware/version.json` | 版本/描述/硬件信息唯一来源 | 见 §3.3 |
| `application_5301/boards/akaLinkPro/board.h` | 尾部保留大小 `BOARD_PARAM_RESERVED_SIZE` | `0x2000`（8 KB） |
| `application_5301/src/easyflash/inc/ef_cfg.h` | `EF_START_ADDR` / `ENV_AREA_SIZE`（= 尾部保留区） | 由 `BOARD_*` 推出 |
| `bootloader_dfu/boards/akaLinkPro/board.h` | `BOARD_DFU_WRITABLE_SIZE` | `BOARD_FLASH_SIZE − 0x2000` |
| `bootloader_dfu/src/dfu_flash_port.c` | DFU 擦除/写入/读取上界 | `< 0x800FE000` |
| `bootloader_dfu/src/dfu_desc.c` | DfuSe 描述的可写区大小 | 扣除 8 KB |

EasyFlash ENV 区地址：
`EF_START_ADDR = 0x80000000 + BOARD_FLASH_SIZE − BOARD_PARAM_RESERVED_SIZE`
`ENV_AREA_SIZE = BOARD_PARAM_RESERVED_SIZE`（= 2 × `EF_ERASE_MIN_SIZE` = 2 × 4 KB）

## 6. 新增自定义数据区的方法

在 **flash 顶端**从 EasyFlash 区继续向下预留（保持 Bootloader 区不动），并同步缩小 APP 区。

### 6.1 示例：新增 2 个扇区（8 KB）自定义数据区

把 APP 区上界从 `0x800FE000` 下调到 `0x800FC000`，自定义区放 `0x800FC000` – `0x800FDFFF`。调整后布局：

| 区域 | 地址范围 | 扇区号 | 大小 |
| --- | --- | --- | --- |
| APP 固件 | `0x80020000` – `0x800FBFFF` | 32 – 251 | 880 KB |
| 自定义数据区（示例） | `0x800FC000` – `0x800FDFFF` | 252 – 253 | 8 KB |
| EasyFlash ENV 区 | `0x800FE000` – `0x800FFFFF` | 254 – 255 | 8 KB |

此时尾部总保留 `TAIL = 4 × 0x1000 = 0x4000`：APP `_flash_size = 0x100000 − 0x4000 = 0xFC000`。

### 6.2 修改清单（模板）

设新增数据区共 `K` 个扇区，尾部总保留 `TAIL = (2 + K) × 0x1000` 字节：

| 文件 | 修改 |
| --- | --- |
| `application_5301/CMakeLists.txt` | `_flash_size = 0x100000 − TAIL` |
| `application_5301/boards/akaLinkPro/board.h` | `BOARD_PARAM_RESERVED_SIZE` → 改用实际尾部总保留大小（或新增 `BOARD_TAIL_RESERVED_SIZE`） |
| `application_5301/src/easyflash/inc/ef_cfg.h` | `EF_START_ADDR`/`ENV_AREA_SIZE` 按新的 EasyFlash 区重算 |
| `bootloader_dfu/boards/akaLinkPro/board.h` | `BOARD_DFU_WRITABLE_SIZE = BOARD_FLASH_SIZE − TAIL` |
| `bootloader_dfu/src/dfu_flash_port.c` | 无需改（引用 `BOARD_DFU_WRITABLE_SIZE`），确保不越界 |
| `bootloader_dfu/src/dfu_desc.c` | DfuSe 描述扣除 `TAIL` |

自定义数据区建议（两种方式）：
- **使用 EasyFlash**：为每个数据区分配独立的 ENV key（如 `"factory"`、`"calib"`），
  由 EasyFlash 统一做 GC / 磨损平衡；地址只需在 `ef_cfg.h` 调整 ENV 区范围与 `default_env_set`。
- **裸 drv_flash**：直接调用 `drv_flash_read/erase/write(addr, buf, size)`（绝对地址，字节粒度），
  自行在区首放 `magic + version + CRC32`，写前先 `drv_flash_erase` 整个扇区。
- 每个独立数据区至少占 1 个 4 KB 扇区（擦除最小单位）。
- 地址计算：`区起始 = 0x80100000 − TAIL + 本区在尾部内的偏移`。

## 7. 快速校验命令（J-Link）

```
device HPM5301xEGx
si JTAG
jtagconf -1 -1
speed 4000
connect
mem8  0x80020000,64     // APP 头：21 4D 50 48 + len + CRC32 + "0.1" + 时间 + 描述
mem8  0x80020100,8      // APP 入口代码（_start）
mem8  0x8001F000,64     // Bootloader 信息块："BLI1" + "1.0" + 时间 + "A.0" + 生产日期
mem8  0x800FE000,16     // EasyFlash 扇区头，含 "EF40" (45 46 34 30)
mem8  0x800FE010,16     // ENV 节点头 "KV40" (4B 56 34 30) + key "cfg"
mem8  0x800FF000,16     // 第二个 ENV 扇区（GC 备用，通常为空）
exit
```
