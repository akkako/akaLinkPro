#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
SWD GPIO ASM 位置无关代码 blob 构建脚本 (Windows / Linux 通用)
用法:
    python build_blob.py [--variant 60M] [--toolchain PATH] [--src FILE] [--clean]
"""

import os
import sys
import subprocess
import argparse
import shutil
import re
from pathlib import Path

# ---------- 默认配置 ----------
DEFAULT_VARIANT = "60M"
DEFAULT_SRC = "../SW_DP_GPIO_ASM_$(VARIANT).S"   # 注意：实际路径会替换变量
TOOLCHAIN_PREFIX = "riscv32-unknown-elf-"
MARCH = "rv32imac_zicsr_zifencei_zba_zbb_zbc_zbs"
MABI = "ilp32"
LINKER_SCRIPT = "blob.ld"

# ---------- 工具路径 ----------
def find_tool(name, toolchain_dir=None):
    """在 toolchain 目录或 PATH 中查找可执行文件"""
    if toolchain_dir:
        exe = Path(toolchain_dir) / (name + ".exe")
        if exe.is_file():
            return str(exe)
        exe = Path(toolchain_dir) / name
        if exe.is_file():
            return str(exe)
    # 查找 PATH
    full = shutil.which(name)
    if full:
        return full
    full = shutil.which(name + ".exe")
    if full:
        return full
    return None

# ---------- 执行命令 ----------
def run_cmd(cmd, cwd=None, check=True, capture=False):
    """执行命令并返回 (returncode, stdout, stderr)"""
    if isinstance(cmd, str):
        cmd = cmd.split()
    if capture:
        proc = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
        return proc.returncode, proc.stdout, proc.stderr
    else:
        proc = subprocess.run(cmd, cwd=cwd, check=check)
        return proc.returncode, "", ""

# ---------- 构建步骤 ----------
class Builder:
    def __init__(self, args):
        self.variant = args.variant
        self.toolchain_dir = args.toolchain
        self.src = args.src
        self.clean = args.clean
        self.build_dir = Path("build")
        self.src_path = Path(self.src)

        # 工具
        self.cc = find_tool(TOOLCHAIN_PREFIX + "gcc", self.toolchain_dir)
        self.ld = find_tool(TOOLCHAIN_PREFIX + "ld", self.toolchain_dir)
        self.objcopy = find_tool(TOOLCHAIN_PREFIX + "objcopy", self.toolchain_dir)
        self.readelf = find_tool(TOOLCHAIN_PREFIX + "readelf", self.toolchain_dir)
        self.nm = find_tool(TOOLCHAIN_PREFIX + "nm", self.toolchain_dir)

        if not all([self.cc, self.ld, self.objcopy, self.readelf, self.nm]):
            missing = []
            if not self.cc: missing.append("gcc")
            if not self.ld: missing.append("ld")
            if not self.objcopy: missing.append("objcopy")
            if not self.readelf: missing.append("readelf")
            if not self.nm: missing.append("nm")
            print(f"错误: 找不到以下工具: {', '.join(missing)}")
            print("请确保 RISC-V 工具链在 PATH 中，或通过 --toolchain 指定 bin 目录")
            sys.exit(1)

        # 中间文件
        self.obj = self.build_dir / f"swd_gpio_asm_{self.variant}.o"
        self.elf = self.build_dir / f"swd_gpio_asm_{self.variant}_blob.elf"
        self.bin = self.build_dir / f"swd_gpio_asm_{self.variant}_blob.bin"
        self.hdr = self.build_dir / f"swd_gpio_asm_{self.variant}_blob.h"
        self.map = self.build_dir / f"swd_gpio_asm_{self.variant}.map"

        # 检查源文件
        if not self.src_path.exists():
            # 尝试替换变量
            alt_src = str(self.src_path).replace("$(VARIANT)", self.variant)
            self.src_path = Path(alt_src)
            if not self.src_path.exists():
                print(f"错误: 源文件不存在: {self.src_path}")
                sys.exit(1)

    def clean_build(self):
        if self.build_dir.exists():
            shutil.rmtree(self.build_dir)
            print("清理完成")

    def ensure_build_dir(self):
        self.build_dir.mkdir(exist_ok=True)

    def build_o(self):
        """编译 .S -> .o"""
        print(f"编译: {self.src_path} -> {self.obj}")
        asflags = [
            "-march=" + MARCH,
            "-mabi=" + MABI,
            "-Ofast",
            "-c",
            "-mno-plt",
            str(self.src_path),
            "-o", str(self.obj)
        ]
        cmd = [self.cc] + asflags
        ret, out, err = run_cmd(cmd, capture=True)
        if ret != 0:
            print("编译失败:")
            print(err)
            sys.exit(ret)

    def build_elf(self):
        """链接 .o -> .elf"""
        print(f"链接: {self.obj} -> {self.elf}")
        # 检查链接脚本是否存在
        if not Path(LINKER_SCRIPT).exists():
            print(f"错误: 链接脚本 {LINKER_SCRIPT} 不存在")
            sys.exit(1)
        ldflags = [
            "-T", LINKER_SCRIPT,
            "-Map", str(self.map),
            "-o", str(self.elf),
            str(self.obj)
        ]
        cmd = [self.ld] + ldflags
        ret, out, err = run_cmd(cmd, capture=True)
        if ret != 0:
            print("链接失败:")
            print(err)
            sys.exit(ret)

    def build_bin(self):
        """提取 .bin"""
        print(f"生成 bin: {self.elf} -> {self.bin}")
        cmd = [self.objcopy, "-O", "binary", str(self.elf), str(self.bin)]
        ret, out, err = run_cmd(cmd, capture=True)
        if ret != 0:
            print("objcopy 失败:")
            print(err)
            sys.exit(ret)

    def build_header(self):
        """从符号表生成头文件"""
        print(f"生成头文件: {self.hdr}")
        # 运行 nm
        cmd = [self.nm, "--defined-only", "-n", str(self.elf)]
        ret, out, err = run_cmd(cmd, capture=True)
        if ret != 0:
            print("nm 失败:")
            print(err)
            sys.exit(ret)

        # 解析 nm 输出
        lines = out.splitlines()
        symbols = {}
        for line in lines:
            if not line.strip():
                continue
            parts = line.split()
            if len(parts) < 3:
                continue
            addr = parts[0]   # 十六进制地址
            typ = parts[1]    # 符号类型
            name = parts[2]   # 符号名称
            # 只关注我们需要的函数
            if "SWJ_Sequence" in name or "SWD_Write" in name or "SWD_Read" in name:
                # 提取前缀（如 SWJ_Sequence、SWD_Write、SWD_Read）
                # 去掉可能的 _GPIO_ASM_ 后缀
                base = name.split("_GPIO_ASM_")[0]
                symbols[base] = addr

        if not symbols:
            print("警告: 未找到任何 SWJ/SWD 符号，头文件可能为空")

        # 生成头文件内容
        content = []
        content.append(f"/* Auto-generated. Offsets of SWD functions inside swd_gpio_asm_{self.variant}_blob.bin */")
        content.append(f"#ifndef SWD_GPIO_ASM_{self.variant}_BLOB_H")
        content.append(f"#define SWD_GPIO_ASM_{self.variant}_BLOB_H")
        content.append("")
        for base, addr in symbols.items():
            # 宏名: 前缀大写 + _OFFSET_ + VARIANT
            macro = base.upper() + f"_OFFSET_{self.variant}"
            content.append(f"#define {macro} 0x{addr}")
        content.append("")
        content.append(f"#endif /* SWD_GPIO_ASM_{self.variant}_BLOB_H */")

        self.hdr.write_text("\n".join(content), encoding="utf-8")
        print(f"头文件生成完成，包含 {len(symbols)} 个符号")

    def build_c_source(self):
        """将 bin 文件转换为 C 语言数组 (.c + .h)"""
        print(f"生成 C 数组: {self.bin} -> {self.build_dir}/swd_gpio_asm_{self.variant}_blob.c/h")
        
        # 读取 bin 文件字节
        if not self.bin.exists():
            print(f"错误: bin 文件不存在 {self.bin}")
            sys.exit(1)
        
        bin_data = self.bin.read_bytes()
        data_len = len(bin_data)
        
        # 生成一个合法的 C 变量名（如 swd_blob_60M）
        var_name = f"swd_blob_{self.variant}"
        
        # ---------- 生成 .h 文件 ----------
        h_content = [
            f"/* Auto-generated. Binary blob for SWD GPIO ASM {self.variant} */",
            f"#ifndef SWD_BLOB_{self.variant}_H",
            f"#define SWD_BLOB_{self.variant}_H",
            "",
            "#include <stdint.h>",
            "#include <stddef.h>",
            "",
            f"extern const uint8_t {var_name}[];",
            f"extern const size_t {var_name}_SIZE;",
            "",
            "#endif /* SWD_BLOB_{self.variant}_H */"
        ]
        h_path = self.build_dir / f"swd_gpio_asm_{self.variant}_blob.h"
        h_path.write_text("\n".join(h_content), encoding="utf-8")
        
        # ---------- 生成 .c 文件 ----------
        # 格式化字节数组，每行 16 个字节
        hex_lines = []
        hex_lines.append(f"/* Auto-generated. Binary blob for SWD GPIO ASM {self.variant} */")
        hex_lines.append("#include <stdint.h>")
        hex_lines.append("#include <stddef.h>")
        hex_lines.append("")
        hex_lines.append(f"const uint8_t {var_name}[] = {{")
        
        # 按 16 字节分组
        for i in range(0, data_len, 16):
            chunk = bin_data[i:i+16]
            hex_str = ", ".join([f"0x{b:02X}" for b in chunk])
            # 如果还有后续，加逗号；最后一行不加（但加了也没事，因为后面还有闭合的大括号）
            line = f"    {hex_str},"
            hex_lines.append(line)
        
        # 关闭数组
        hex_lines.append("};")
        hex_lines.append("")
        hex_lines.append(f"const size_t {var_name}_SIZE = {data_len};")
        
        c_path = self.build_dir / f"swd_gpio_asm_{self.variant}_blob.c"
        c_path.write_text("\n".join(hex_lines), encoding="utf-8")
        
        print(f"  C 数组大小: {data_len} 字节")
        
    def run_checks(self):
        """检查重定位"""
        print("检查重定位...")
        # 检查 .o 文件
        cmd = [self.readelf, "-r", str(self.obj)]
        ret, out, err = run_cmd(cmd, capture=True)
        if ret != 0:
            print("readelf 失败:")
            print(err)
            sys.exit(ret)

        bad_types = ["R_RISCV_32", "R_RISCV_64", "R_RISCV_HI20", "R_RISCV_LO12",
                     "R_RISCV_CALL", "R_RISCV_JAL", "R_RISCV_GOT"]
        found = []
        for line in out.splitlines():
            for typ in bad_types:
                if typ in line:
                    found.append(line.strip())
        if found:
            print("错误: .o 文件中发现绝对重定位，不可搬运!")
            for l in found:
                print("  " + l)
            sys.exit(1)
        else:
            print("  .o 检查通过: 无绝对重定位")

        # 检查 .elf 文件（链接后应无任何重定位）
        cmd = [self.readelf, "-r", str(self.elf)]
        ret, out, err = run_cmd(cmd, capture=True)
        if ret != 0:
            # readelf 对无重定位的 elf 可能返回 0，但有时返回非零？我们只检查输出
            pass
        if out.strip():
            # 如果有输出，说明可能有重定位
            if any("R_RISCV" in line for line in out.splitlines()):
                print("错误: 链接后的 ELF 仍含有重定位，不可用!")
                print(out)
                sys.exit(1)
        print("  ELF 检查通过: 无重定位残留")

    def run(self):
        if self.clean:
            self.clean_build()
            return
        self.ensure_build_dir()
        self.build_o()
        self.build_elf()
        self.build_bin()
        self.build_c_source()
        self.build_header()
        self.run_checks()
        print("构建成功!")
        print(f"  BIN: {self.bin}")
        print(f"  HDR: {self.hdr}")
        print(f"  SRC: {self.build_dir / f'swd_gpio_asm_{self.variant}_blob.c'}")


def main():
    parser = argparse.ArgumentParser(description="SWD GPIO ASM blob 构建脚本")
    parser.add_argument("--variant", default=DEFAULT_VARIANT,
                        help=f"速度版本 (默认 {DEFAULT_VARIANT})")
    parser.add_argument("--toolchain", help="RISC-V 工具链 bin 目录路径")
    parser.add_argument("--src", default=DEFAULT_SRC,
                        help="汇编源文件路径 (支持 $(VARIANT) 占位符)")
    parser.add_argument("--clean", action="store_true", help="清理 build 目录")
    args = parser.parse_args()

    # 处理源文件中的 $(VARIANT)
    if "$(VARIANT)" in args.src:
        args.src = args.src.replace("$(VARIANT)", args.variant)

    builder = Builder(args)
    builder.run()

if __name__ == "__main__":
    main()