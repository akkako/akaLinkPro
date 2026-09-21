# akaLinkPro

akaLinkPro 是一个基于 HPM5300 的高性能 CMSIS-DAP 调试器。

- 主固件：`firmware/application_5301`
- DFU/MSC Bootloader：`firmware/bootloader_dfu`
- 配置上位机（WebHID）：`docs/index.html`

构建 / 烧录 / 调试说明见 `.opencode/skills/akalinkpro-firmware/SKILL.md`。

## License

本项目采用 **Apache License 2.0** 许可，详见根目录 [`LICENSE`](LICENSE)。

```
Copyright (c) 2026 akaInstruments

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0
```

## 第三方组件

本项目在源码与构建中使用/参考了以下第三方组件，其版权归各自作者所有，
并按其各自的许可证分发（这些组件的原始许可证声明请见对应文件/目录）：

| 组件 | 用途 | 许可证 |
| --- | --- | --- |
| [HPMicro HPM SDK](https://github.com/hpmicro/hpm_sdk) | HPM5301 SoC/驱动/启动/链接脚本 | BSD-3-Clause |
| [ARM CMSIS-DAP](https://github.com/ARM-software/CMSIS-DAP) | CMSIS-DAP 命令引擎（`src/dap/`） | Apache-2.0 |
| [CherryUSB](https://github.com/cherry-embedded/CherryUSB) | USB 设备协议栈 | Apache-2.0 |
| [CherryRB](https://github.com/cherry-embedded/CherryRB) | 无锁环形缓冲区 | Apache-2.0 |
| [EasyFlash](https://github.com/armink/EasyFlash) | 参数持久化（`src/easyflash/`） | MIT |

> 若再分发二进制，请一并保留上述组件的版权与许可证声明。
