# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 akaInstruments

#!/usr/bin/env python3
"""Firmware image post-build packer for akaLink Pro (HPM5301).

Injects version / build-time / description metadata and the APP integrity
header (length + CRC32) into the built images.

APP header (256 B @ 0x80020000), code starts at 0x80020100:
    +0x00  4  DFU signature 0x48504D21 (untouched)
    +0x04  4  app code length
    +0x08  4  app CRC32 (seed 0x0D000721)
    +0x0C  4  header format version (=1)
    +0x10  8  firmware version string
    +0x18 20  firmware build time  "YYYY/MM/DD HH:MM:SS"
    +0x2C 24  firmware description
    +0x44 ... reserved

Bootloader info block (256 B @ 0x8001F000):
    +0x00  4  magic "BLI1" (0x31494C42)
    +0x04  4  bootloader code length
    +0x08  4  bootloader CRC32 (unused)
    +0x0C  4  format version (=1)
    +0x10  8  bootloader version string
    +0x18 20  bootloader build time
    +0x2C  8  hardware version string
    +0x34 20  hardware production date
"""
import argparse
import binascii
import json
import os
import struct
from datetime import datetime

APP_BASE = 0x80020000
APP_HEADER_SIZE = 0x100
BL_INFO_BASE = 0x8001F000
CRC_SEED = 0x0D000721

APP_OFF_LEN = 0x04
APP_OFF_CRC = 0x08
APP_OFF_HDRVER = 0x0C
APP_OFF_FWVER = 0x10
APP_OFF_FWTS = 0x18
APP_OFF_DESC = 0x2C

BL_OFF_MAGIC = 0x00
BL_OFF_LEN = 0x04
BL_OFF_CRC = 0x08
BL_OFF_FMTVER = 0x0C
BL_OFF_BLVER = 0x10
BL_OFF_BLTS = 0x18
BL_OFF_HWVER = 0x2C
BL_OFF_HWPROD = 0x34


def fixed_str(s, n):
    b = s.encode('ascii')[:n]
    return b + b'\x00' * (n - len(b))


def crc32(data):
    return binascii.crc32(data, CRC_SEED) & 0xFFFFFFFF


def build_time_19():
    return datetime.now().strftime('%Y/%m/%d %H:%M:%S')


def ihex_lines(segments):
    """Encode [(base_addr, bytes)] into Intel HEX record lines (without EOF)."""
    lines = []
    upper = None
    for base, data in segments:
        addr = base
        i = 0
        while i < len(data):
            chunk = data[i:i + 16]
            hi = (addr >> 16) & 0xFFFF
            if hi != upper:
                upper = hi
                rec = bytes([0x02, 0x00, 0x00, 0x04, (hi >> 8) & 0xFF, hi & 0xFF])
                lines.append(_hex_record(rec))
            rec = bytes([len(chunk), (addr >> 8) & 0xFF, addr & 0xFF, 0x00]) + chunk
            lines.append(_hex_record(rec))
            addr += len(chunk)
            i += len(chunk)
    return lines


def _hex_record(rec):
    checksum = (-sum(rec)) & 0xFF
    return ':' + rec.hex().upper() + '%02X' % checksum


def load_versions(path):
    with open(path, 'r', encoding='utf-8') as f:
        v = json.load(f)
    for key in ('app_fw_ver', 'app_desc', 'bl_ver', 'hw_ver', 'hw_prod_date'):
        v.setdefault(key, '')
    return v


def pack_app(args):
    ver = load_versions(args.version_json)
    ts = build_time_19()

    with open(args.bin, 'rb') as f:
        raw = f.read()
    if len(raw) <= APP_HEADER_SIZE:
        raise SystemExit('app bin too small: %d' % len(raw))

    header = bytearray(raw[:APP_HEADER_SIZE])
    code = raw[APP_HEADER_SIZE:]

    struct.pack_into('<I', header, APP_OFF_LEN, len(code))
    struct.pack_into('<I', header, APP_OFF_CRC, crc32(code))
    struct.pack_into('<I', header, APP_OFF_HDRVER, 1)
    header[APP_OFF_FWVER:APP_OFF_FWVER + 8] = fixed_str(ver['app_fw_ver'], 8)
    header[APP_OFF_FWTS:APP_OFF_FWTS + 20] = fixed_str(ts, 20)
    header[APP_OFF_DESC:APP_OFF_DESC + 24] = fixed_str(ver['app_desc'], 24)

    out = bytes(header) + code
    with open(args.out_bin, 'wb') as f:
        f.write(out)
    if args.out_hex:
        hex_text = '\n'.join(ihex_lines([(APP_BASE, out)])) + '\n:00000001FF\n'
        with open(args.out_hex, 'w', encoding='ascii') as f:
            f.write(hex_text)

    print('[pack app] ver=%s len=%d (0x%X) crc32=0x%08X time=%s' %
          (ver['app_fw_ver'], len(code), len(code), crc32(code), ts))
    print('[pack app] out: %s%s' % (args.out_bin, (' / ' + args.out_hex) if args.out_hex else ''))


def pack_boot(args):
    ver = load_versions(args.version_json)
    ts = build_time_19()

    code_len = 0
    if args.code_bin and os.path.exists(args.code_bin):
        code_len = os.path.getsize(args.code_bin)

    meta = bytearray(APP_HEADER_SIZE)
    meta[BL_OFF_MAGIC:BL_OFF_MAGIC + 4] = b'BLI1'
    struct.pack_into('<I', meta, BL_OFF_LEN, code_len)
    struct.pack_into('<I', meta, BL_OFF_CRC, 0)
    struct.pack_into('<I', meta, BL_OFF_FMTVER, 1)
    meta[BL_OFF_BLVER:BL_OFF_BLVER + 8] = fixed_str(ver['bl_ver'], 8)
    meta[BL_OFF_BLTS:BL_OFF_BLTS + 20] = fixed_str(ts, 20)
    meta[BL_OFF_HWVER:BL_OFF_HWVER + 8] = fixed_str(ver['hw_ver'], 8)
    meta[BL_OFF_HWPROD:BL_OFF_HWPROD + 20] = fixed_str(ver['hw_prod_date'], 20)

    # Start from the linker-generated bootloader hex, drop its EOF record.
    with open(args.hex, 'r', encoding='ascii') as f:
        lines = [ln.strip() for ln in f if ln.strip()]
    lines = [ln for ln in lines if not ln.startswith(':00000001FF')]
    meta_lines = ihex_lines([(BL_INFO_BASE, bytes(meta))])
    out_lines = lines + meta_lines + [':00000001FF']

    with open(args.out_hex, 'w', encoding='ascii') as f:
        f.write('\n'.join(out_lines) + '\n')

    print('[pack boot] bl_ver=%s hw=%s prod=%s len=%d time=%s' %
          (ver['bl_ver'], ver['hw_ver'], ver['hw_prod_date'], code_len, ts))
    print('[pack boot] out: %s' % args.out_hex)


def main():
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest='target', required=True)

    p_app = sub.add_parser('app')
    p_app.add_argument('--bin', required=True)
    p_app.add_argument('--out-bin', required=True)
    p_app.add_argument('--out-hex', default='')
    p_app.add_argument('--version-json', required=True)
    p_app.set_defaults(func=pack_app)

    p_boot = sub.add_parser('boot')
    p_boot.add_argument('--hex', required=True)
    p_boot.add_argument('--code-bin', default='')
    p_boot.add_argument('--out-hex', required=True)
    p_boot.add_argument('--version-json', required=True)
    p_boot.set_defaults(func=pack_boot)

    args = parser.parse_args()
    args.func(args)


if __name__ == '__main__':
    main()
