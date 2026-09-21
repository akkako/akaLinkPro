# HPM5301 固件完整性校验 / 版本信息嵌入 方案计划

> 参考工程：`X:\cproject\Debugger\akaLinkCompact\firmware`（CH32V305 CMSIS-DAP）
> 目标工程：`X:\cproject\Debugger\akaLinkPro\firmware`（HPM5301 CMSIS-DAP）
> 状态：**已实施并自检通过**（见 §8）

---

## 0. 已确认决策（评审记录）

| # | 议题 | 决策 |
| --- | --- | --- |
| 1 | APP 完整性校验主体 | **仅由 Bootloader 校验**；校验失败**停留在 Bootloader（DFU）** |
| 2 | Bootloader 自身校验 | **不做** |
| 3 | 版本/描述/硬件版本配置源 | **待定**（见 §7-Q3） |
| 4 | 编译时间格式 | `YYYY/MM/DD HH:MM:SS` |
| 5 | 头/信息块大小、入口偏移 | **256 B 头、入口 `+0x100`**；已验证 HPM 可支持（见 §4.4） |
| 6 | Bootloader 信息块位置 | Bootloader 区尾 `0x8001F000` |
| 7 | APP 链接方式 | **自定义链接脚本**（`linker/flash_dfu_app.ld`） |
| 8 | 校验失败的状态提示 | 预留状态变量，后续再定义 |

### 4.4 可行性验证结果（入口 `+0x100`）

已用自定义链接脚本原型验证（`CUSTOM_GCC_LINKER_FILE=linker/flash_dfu_app.ld`，`.start` 中签名后 `. = ALIGN(0x100)`）：

| 检查项 | 结果 |
| --- | --- |
| ELF Entry point | `0x80020100` ✅ |
| `.start` 段 | `0x80020000`，大小 `0x150`（0x100 头 + 0x50 代码）✅ |
| `dfu_signature` | `0x80020000` = `21 4D 50 48` ✅ |
| `_start` | `0x80020100` ✅ |
| `0x80020004`–`0x800200FF` | 链接器填充 `0x00`，由 `pack` 覆写 ✅ |
| 编译 | 通过（FLASH 888 KB 区） ✅ |

**结论：HPM5301 支持 256 B 头 + 入口 `0x100` 的方案，沿用 `0x100`。**

---

## 1. 参考工程 akaLinkCompact (CH32V305) 调研

### 1.1 Flash 布局与地址选择

CH32V305 为 **128 KB Flash**（`0x00000000`–`0x0001FFFF`），规划如下：

| 区域 | 起始 | 结束 | 大小 | 说明 |
| --- | --- | --- | --- | --- |
| Bootloader 代码 | `0x00000000` | `0x00003EE3` | ~15.7 KB | 链接区 `LENGTH=0x3EE4` |
| Bootloader 版本串 | `0x00003EE4` | `0x00003EEB` | 8 B | `pack.py` 写入 |
| Bootloader 编译时间 | `0x00003EEC` | `0x00003EFF` | 20 B | `YYYY/MM/DD HH:MM:SS` + `\0` |
| 硬件版本串 | `0x00003F00` | `0x00003F07` | 8 B | 板级信息 |
| 硬件生产日期 | `0x00003F08` | `0x00003F1B` | 20 B | 板级信息 |
| （填充至 16 KB） | `0x00003F1C` | `0x00003FFF` | — | 0xFF |
| **APP 头** | `0x00004000` | `0x000040FF` | 256 B | 长度/CRC/版本/日期/描述 |
| APP 代码 | `0x00004100` | `0x0001FEFF` | 114 KB | 链接区 `ORIGIN=0x4100` |
| 参数存储页 | `0x0001FF00` | `0x0001FFFF` | 256 B | 单页配置 |

**APP 头结构（256 B，`pack.py` 生成）**

| 偏移 | 长度 | 字段 |
| --- | --- | --- |
| `0x00` | 4 | APP 代码长度 `app_len`（LE） |
| `0x04` | 4 | APP 代码 CRC32（LE） |
| `0x08` | 8 | 固件版本串 |
| `0x10` | 20 | 固件编译时间 |
| `0x24` | 24 | 固件描述串 |
| `0x3C`–`0xFF` | — | 0x00/0xFF 填充 |

> 注意：APP 头**不在** ELF 链接镜像内；APP 链接到 `0x4100`，`pack.py` 在二进制前**追加** 256 B 头，烧录到 `0x4000`，因此代码落到 `0x4100`。

### 1.2 信息嵌入机制

构建后处理脚本 `Bootloader/proj/pack.py`、`Application/proj/pack.py`（Makefile 的 `POST_BUILD` 调用）：

| 脚本 | 动作 |
| --- | --- |
| Bootloader `pack.py` | 读取原始 bin → 截断到 `0x3EE4` → 在 `0x3EE4/0x3EEC/0x3F00/0x3F08` 写入版本/时间 → 补足到 `0x4000` |
| Application `pack.py` | 计算 `app_len` 与 CRC32 → 生成 256 B 头 + 代码，输出 `*_pack.bin` |

- 版本号、描述在 `pack.py` 里以常量配置（`bl_ver_str='1.02'`、`fw_ver_str='0.02'`、`hw_ver_str='A.01'`、`desc_str`）。
- 编译时间取**打包时刻** `datetime.now()`，格式 `%Y/%m/%d %H:%M:%S`。
- CRC32 种子 `0x0D000721`，算法等价于 Python `binascii.crc32(data, 0x0D000721)`。

### 1.3 Bootloader 完整性校验流程

`Bootloader/app/main.c::check_app_integrity()`：

1. 读 `0x4000` 得 `app_length`；若 `> APPLICATION_MAX_SIZE` → 判定失败；
2. 读 `0x4004` 得 `file_crc32`；
3. `calc = crc32(0x0D000721, (uint8_t*)0x4100, app_length)`；
4. `calc == file_crc32` 则校验通过。

Bootloader 主流程：校验失败 / BKP 触发 / 看门狗异常复位 三者任一 → 停留在 Bootloader（USB MSC/FAT16 升级）；否则跳转 APP（软件中断跳到 `0x4100`）。

### 1.4 APP 侧读取方式

APP 直接以**绝对地址**读取元数据（`sys_config.h`）：

| 宏 | 地址 | 内容 |
| --- | --- | --- |
| `BOOTLOADER_VER_STR_ADDR` | `0x3EE4` | Bootloader 版本 |
| `BOOTLOADER_TS_STR_ADDR` | `0x3EEC` | Bootloader 编译时间 |
| `HARDWARE_VER_STR_ADDR` | `0x3F00` | 硬件版本 |
| `HARDWARE_PROD_TS_STR_ADDR` | `0x3F08` | 生产日期 |
| `APPLICATION_VER_STR_ADDR` | `0x4008` | 固件版本 |
| `APPLICATION_TS_STR_ADDR` | `0x4010` | 固件编译时间 |
| `APPLICATION_DESC_STR_ADDR` | `0x4024` | 固件描述 |

HID 的 `0x12`–`0x17` 指令即从这些地址取字符串返回；App 自身配置存于最后一页 `0x1FF00`。

### 1.5 小结（可复用的设计点）

1. **固定地址元数据块**：Bootloader 信息块 + APP 头块，构建后由 `pack.py` 填充。
2. **APP 头在代码之前 256 B**，长度 + CRC + 版本/时间/描述。
3. **Bootloader 启动时校验 APP 长度 + CRC32**，失败则留在 Bootloader。
4. **CRC32 带固定种子**，PC 端与 MCU 端算法一致。
5. 编译时间/版本在**打包阶段**统一注入，固件源码不硬编码。

---

## 2. HPM5301 工程现状

### 2.1 当前 Flash 布局（1 MB QSPI NOR，`0x80000000`）

| 区域 | 地址范围 | 大小 | 说明 |
| --- | --- | --- | --- |
| Bootloader | `0x80000000`–`0x8001FFFF` | 128 KB | ROM 由 `nor_cfg_option@0x400`、`boot_header@0x1000` 引导；实际代码 ~40 KB |
| APP | `0x80020000`–`0x800FDFFF` | 888 KB | 链接区；实际 ~81 KB |
| EasyFlash 配置区 | `0x800FE000`–`0x800FFFFF` | 8 KB | 2×4 KB 扇区，磨损平衡 |

### 2.2 与参考工程的差异 / 现状问题

| 项 | CH32V305 | HPM5301 现状 |
| --- | --- | --- |
| APP 头部 | `0x4000` 256 B 头 + `0x4100` 代码 | **无**；首 4 字节仅 DFU 签名 `0x48504D21`，入口 `+4` |
| APP 完整性校验 | Bootloader 校验长度+CRC | **无**（仅校验 4 字节签名） |
| 版本/日期 | 打包注入，绝对地址读取 | **源码硬编码字符串**（`api_param.c` 宏） |
| Bootloader 信息 | `0x3EE4` 起 | **无** |
| 硬件版本/生产日期 | Bootloader 区 | **源码硬编码** |
| 参数存储 | 最后一页 | EasyFlash（已实现，2 扇区） |

> HPM ROM 只引导 **Bootloader**（`boot_header`），APP 完全由 Bootloader 跳转，因此 APP 头/入口可自由规划。

---

## 3. 目标

1. APP 固件嵌入：**长度 + CRC32 + 版本 + 编译日期时间 + 描述**，固定地址可读；
2. Bootloader 嵌入：**版本 + 编译日期时间**；板级：**硬件版本 + 生产日期**；
3. Bootloader 启动时**校验 APP 完整性**（长度 + CRC32），失败则停留在 Bootloader 便于升级；
4. APP 侧 HID `0x12`–`0x17` 改为读取上述元数据；
5. 不破坏现有 DFU、EasyFlash、尾部 8 KB 保留与 J-Link 流程。

---

## 4. 建议方案（推荐方案 A）

### 4.1 新地址分布

| 区域 | 地址范围 | 大小 | 说明 |
| --- | --- | --- | --- |
| Bootloader 代码 | `0x80000000`–`0x8001EFFF` | < 124 KB | 链接区，正常 ~40 KB |
| **Bootloader 信息块** | `0x8001F000`–`0x8001F0FF` | 256 B | 版本/编译时间/硬件版本/生产日期（扇区其余保留） |
| **APP 头** | `0x80020000`–`0x800200FF` | 256 B | `0x00` DFU 签名 + 长度 + CRC + 版本/时间/描述 |
| APP 代码 | `0x80020100`–`0x800FDFFF` | ~888 KB | 链接入口 `0x80020100` |
| EasyFlash 配置区 | `0x800FE000`–`0x800FFFFF` | 8 KB | 不变 |

### 4.2 APP 头结构（`0x80020000`，256 B）

| 偏移 | 长度 | 字段 | 说明 |
| --- | --- | --- | --- |
| `0x00` | 4 | DFU 签名 | `0x48504D21`（"HPM!"），**保持不变** |
| `0x04` | 4 | `app_code_len` | 代码字节数（从 `0x80020100` 起） |
| `0x08` | 4 | `app_crc32` | CRC32（种子 `0x0D000721`，仅覆盖代码） |
| `0x0C` | 4 | `header_ver` | 头格式版本 = 1 |
| `0x10` | 8 | `fw_ver_str` | 固件版本，如 `0.1`+\0 |
| `0x18` | 20 | `fw_build_time` | `YYYY/MM/DD HH:MM:SS`+\0 |
| `0x2C` | 24 | `fw_desc` | 如 `akaLinkPro CMSIS-DAP` |
| `0x44` | 0xBC | reserved | 0xFF |

### 4.3 Bootloader 信息块（`0x8001F000`，256 B）

| 偏移 | 长度 | 字段 |
| --- | --- | --- |
| `0x00` | 4 | magic `"BLI1"` = `0x31494C42` |
| `0x04` | 4 | `bl_code_len` |
| `0x08` | 4 | `bl_crc32`（可选） |
| `0x0C` | 4 | `format_ver` = 1 |
| `0x10` | 8 | `bl_ver_str` |
| `0x18` | 20 | `bl_build_time` |
| `0x2C` | 8 | `hw_ver_str` |
| `0x34` | 20 | `hw_prod_date` |
| `0x48` | 0xB8 | reserved |

### 4.4 APP 链接与入口调整（推荐：自定义链接脚本）

- 复制 SDK `flash_dfu.ld` 到工程（如 `application_5301/linker/flash_dfu_app.ld`），仅改 `.start`：

```
.start : {
    KEEP(*(.dfu_signature))     /* 0x80020000 : 4 B 签名 */
    . = ALIGN(0x100);           /* 保留 0x80020004-0x800200FF 给 APP 头 */
    KEEP(*(.start))             /* 0x80020100 : 入口 */
    . = ALIGN(8);
} > FLASH
```

- `CMakeLists.txt` 设置 `CUSTOM_GCC_LINKER_FILE=<绝对路径>`（SDK 支持，已验证机制）。
- APP 入口由 `0x80020004` 变为 `0x80020100`；APP 自身可用固定地址直接读自己的头。
- 保留 `_flash_size=0xFE000` 尾部 8 KB 预留。

> 备选方案 B：不改链接脚本，把 `_dfu_bl_length` 改为 `0x20100` 使 APP 链接到 `0x80020100`，
> 去掉 ELF 中的 `.dfu_signature`，由 `pack.py` 在镜像前追加 256 B 头（与 CH32V 完全一致）。
> 缺点：APP ELF 不再包含头/签名，且需移除 board.c 的 `.dfu_signature`。

### 4.5 构建注入流程（pack）

新增打包脚本（如 `application_5301/proj/pack_app.py`、`bootloader_dfu/proj/pack_bl.py`），由 CMake `POST_BUILD` 调用：

| 目标 | 输入 | 处理 | 输出 |
| --- | --- | --- | --- |
| APP | `akaLinkPro_App.bin`（含头占位） | `app_code_len = size-0x100`；CRC32(代码)；填头字段 版本/时间/描述 | `..._pack.bin` / `..._pack.hex` |
| Bootloader | `akaLinkPro_Boot.bin` | 在 `0x8001F000` 写信息块（版本/时间/硬件版本/生产日期） | `..._pack.bin` / `..._pack.hex` |

- 版本号等集中配置（建议 `proj/version.json` 或 CMake 变量，供两脚本读取），编译时间取打包时刻。
- CRC 与 MCU 端一致：`binascii.crc32(data, 0x0D000721)`。
- hex 由「打包后的 bin」转换（`objcopy -I binary -O ihex --change-addresses <base>`），保证 J-Link/dfu-util 使用同一镜像。

### 4.6 Bootloader 校验流程（改动点）

`bootloader_dfu/src/hpm_dfu_trigger.c::hpm_dfu_check_bootloader_request()`：

```
#define APP_BASE      0x80020000
#define APP_CODE      (APP_BASE + 0x100)
#define APP_MAX_LEN   (0x800FE000 - APP_CODE)     /* 0xDDF00 */

读 0x80020000 == BOARD_DFU_SIGNATURE ? 否则停留 DFU
len = *(u32*)(APP_BASE+4); crc = *(u32*)(APP_BASE+8);
若 len==0 || len>APP_MAX_LEN || header_ver!=1        -> 失败，停留 DFU
l1c_dc_invalidate(APP_CODE, len)
crc32_seed(0x0D000721, APP_CODE, len) == crc ? 成功 -> 跳转 APP_CODE
```

- 跳转地址 `hpm_dfu_jump_to_app()` 由 `+4` 改为 `+0x100`。
- 新增一个带种子的 CRC32 实现（可参考 SDK `hpm_crc32.c`，加种子与末尾取反）。
- 校验失败行为：**停留在 Bootloader（DFU 模式）**，便于重新烧录。

### 4.7 APP 侧读取（HID 改造）

`src/api/api_param.c` 中：

- `APPLICATION_VER_STR_ADDR` → `0x80020010`
- `APPLICATION_TS_STR_ADDR` → `0x80020018`
- `APPLICATION_DESC_STR_ADDR` → `0x8002002C`
- `BOOTLOADER_VER_STR_ADDR` → `0x8001F010`
- `BOOTLOADER_TS_STR_ADDR` → `0x8001F018`
- `HARDWARE_VER_STR_ADDR` → `0x8001F02C`
- `HARDWARE_PROD_TS_STR_ADDR` → `0x8001F034`

读取方式：直接 XIP 读（必要时 `l1c_dc_invalidate`），或经 `drv_flash_read`。HID `0x12`–`0x17` 逻辑不变。

### 4.8 烧录流程影响

| 方式 | 现状 | 改后 |
| --- | --- | --- |
| J-Link APP | `loadfile App.hex` | `loadbin App_pack.bin,0x80020000`（或 pack 后的 hex） |
| J-Link Bootloader | `loadfile Boot.hex` | `loadbin Boot_pack.bin,0x80000000`（含 `0x8001F000` 信息块） |
| dfu-util | `-D App.bin -s 0x80020000:leave` | `-D App_pack.bin` |
| DFU 描述 | 可写区结尾扣 8 KB | 不变 |

---

## 5. 实施步骤（建议顺序）

1. **APP 链接脚本**：新增自定义 `flash_dfu_app.ld` + `CUSTOM_GCC_LINKER_FILE`，把入口改到 `0x80020100`，验证可正常启动（Bootloader 临时改跳 `+0x100`）。
2. **Bootloader 信息块 + APP 头写入**：编写 `pack_bl.py` / `pack_app.py`，接入 CMake POST_BUILD。
3. **Bootloader 校验**：加入长度 + CRC32 校验，失败停留 DFU；更新跳转地址。
4. **APP 读取改造**：HID `0x12`–`0x17` 改为读固定地址元数据。
5. **工具链脚本**：更新 `flash_jlink.bat`、`build.bat`/`build_dfu.bat`、`dfu_desc.c` 说明。
6. **文档**：更新 `Flash_Memory_Map.md`、`Custom HID Protocol.md`（如版本字段来源变化）。
7. **回归测试**：正常启动、CRC 破坏后停留 DFU、DFU 升级、EasyFlash 配置持久化、HID 版本读取。

---

## 6. 风险与注意事项

1. **入口迁移**：改链接脚本后必须确认 `_start`/向量表加载地址正确（`.vectors` 的 `AT>` 紧随 `.start`）。
2. **CRC 覆盖范围**：仅覆盖代码区（`0x80020100` 起 `app_code_len`），PC 端与 MCU 端必须完全一致；bin 空隙填充需确定（0x00/0xFF）。
3. **Cache 一致性**：校验前 `l1c_dc_invalidate`，写入后 `fence.i`（现有 drv/DFU 已有）。
4. **信息块扇区保护**：`0x8001F000` 属 Bootloader 区，APP DFU 不会写；确认 Bootloader 代码不越界（当前 ~0x9E00）。
5. **J-Link hex/bin 一致性**：确保 J-Link 与 dfu-util 使用**打包后**的镜像。
6. **版本注入时机**：编译时间以打包时刻为准，避免 C 源码 `__DATE__` 与头内时间不一致。
7. **失败策略**：校验失败停留 DFU；若希望仍可启动需提供「强制启动」入口（可选）。
8. **兼容旧 Bootloader**：Bootloader 与 APP 必须**成对升级**（入口/头格式变更）。

---

## 7. 剩余待确认

- **Q3 版本/描述/硬件版本的唯一配置源**：已采用 `firmware/version.json`
  （字段 `app_fw_ver`、`app_desc`、`bl_ver`、`hw_ver`、`hw_prod_date`），由 app/bootloader 两个
  pack 流程共同读取。

## 8. 实施结果 / 自检

| 项 | 结果 |
| --- | --- |
| APP 自定义链接脚本 | ✅ `linker/flash_dfu_app.ld`，Entry=`0x80020100`，`.start`=`0x80020000`+`0x150` |
| APP 打包 | ✅ `pack.py app` 注入 len/CRC/ver/time/desc，输出 `_pack.bin`/`_pack.hex` |
| Bootloader 打包 | ✅ `pack.py boot` 在 `0x8001F000` 写入信息块，输出 `_pack.hex` |
| Bootloader 校验 | ✅ 签名 + 长度 + CRC32；跳转 `0x80020100` |
| 正常启动 | ✅ 烧录后枚举 `VID_0D28 PID_0204`（APP 运行） |
| 完整性失败 | ✅ 破坏 APP 头/代码后复位 → 停留 Bootloader（`PID_0205` DFU） |
| 恢复 | ✅ 重新烧录 packed APP 后恢复正常 |
| dfu-util 升级 | ✅ `-D _pack.bin -s 0x80020000:leave` 成功，重启后 APP 运行（校验通过） |
| HID 读取元数据 | ✅ `0x12`/`0x13`/`0x14`/`0x15`/`0x16`/`0x17` 分别返回 `A.0`/`0.1`/`1.0`/`2026-08-06`/`2026/09/21 19:50:23`/`2026/09/21 19:50:36` |
| EasyFlash 配置 | ✅ `GET_CONFIG` 返回 `output=0,v5=1,clock=0,led1=4,led2=5,vref=3300`（未受影响） |
| 烧录脚本 | ✅ `flash_jlink.bat`(app/boot)、`build.bat`、`program.bat` 改用 `_pack` 产物 |
| 文档 | ✅ `Flash_Memory_Map.md` 更新（§1/§3/§3.1/§3.2/§3.3/§5/§7） |

未完成（按决策留待后续）：
- 校验失败状态变量 / 指示灯提示（决策 8）。
- Bootloader 自身 CRC（决策 2）。
- APP 自校验（决策 1）。
