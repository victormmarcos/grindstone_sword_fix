#!/usr/bin/env python3
"""Validate the public archive before release."""

from __future__ import annotations

import argparse
import re
import struct
import sys
import zipfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import project_version  # noqa: E402

VERSION = project_version.PACKED
DLL = "SKSE/Plugins/GrindstoneSwordFix.dll"
EXPECTED_RUNTIMES = [0x01050610, 0x010613D0, 0x010613E0, 0x01061430, 0x01061560,
    0x01061610, 0x01062750, 0x01062800, 0x01062931, 0x010646A0,
    0x01064920, 0x010649B1, 0x01070630, 0x01070680]
PROFILE_ROOT = re.compile(rb"[A-Za-z]:[\\/]Users[\\/][^\\/\x00]+", re.I)


def validate_dll(data: bytes) -> None:
    u16 = lambda at: struct.unpack_from("<H", data, at)[0]
    u32 = lambda at: struct.unpack_from("<I", data, at)[0]
    pe = u32(0x3C)
    assert data[:2] == b"MZ" and data[pe:pe + 4] == b"PE\0\0", "Not a PE image"
    assert u16(pe + 4) == 0x8664, "Not an x64 image"
    opt = pe + 24
    sec = opt + u16(pe + 20)
    sections = [(u32(sec + i * 40 + 12), u32(sec + i * 40 + 8), u32(sec + i * 40 + 20), u32(sec + i * 40 + 16))
                for i in range(u16(pe + 6))]

    def offset(rva: int) -> int:
        for va, size, raw, raw_size in sections:
            if va <= rva < va + max(size, raw_size):
                return raw + rva - va
        raise AssertionError(f"Unmapped RVA {rva:x}")

    def string(at: int) -> str:
        return data[at:data.index(b"\0", at)].decode("ascii")

    export = offset(u32(opt + 112))
    names, ordinals, funcs = offset(u32(export + 32)), offset(u32(export + 36)), offset(u32(export + 28))
    exports = {string(offset(u32(names + i * 4))): offset(u32(funcs + u16(ordinals + i * 2) * 4))
               for i in range(u32(export + 24))}
    assert {"SKSEPlugin_Version", "SKSEPlugin_Query", "SKSEPlugin_Load"} <= exports.keys()
    version = exports["SKSEPlugin_Version"]
    assert u32(version + 4) == VERSION, "Wrong mod version in shipped DLL"
    assert u32(version + 0x308) == 0, "Explicit runtime allowlist expected"
    runtimes = [u32(version + 0x30C + i * 4) for i in range(16)]
    assert runtimes == EXPECTED_RUNTIMES + [0, 0], "Wrong runtime/store identifiers in shipped DLL"
    debug_size = u32(opt + 112 + 6 * 8 + 4)
    if debug_size:
        debug = offset(u32(opt + 112 + 6 * 8))
        kinds = [u32(debug + i + 12) for i in range(0, debug_size, 28)]
        assert 2 not in kinds, "Release must not ship a CodeView/PDB path"
    assert b"Installed AnimationObjects::Load hook." in data
    assert b"AnimObjectR" in data and b"Scb" in data


def validate(path: Path) -> None:
    with zipfile.ZipFile(path) as archive:
        names = {info.filename for info in archive.infolist() if not info.is_dir()}
        assert names == {DLL, "LICENSE.txt"}, f"Unexpected package entries: {sorted(names)}"
        assert b"GNU GENERAL PUBLIC LICENSE" in archive.read("LICENSE.txt")[:200], "LICENSE.txt is not the GPL-3.0 text"
        data = archive.read(DLL)
        validate_dll(data)
        assert not PROFILE_ROOT.search(data.replace(b"\x00", b"")), "Private profile root in DLL"
    print(f"Validated {path}: one DLL + LICENSE.txt, version {project_version.TEXT}, {len(EXPECTED_RUNTIMES)} runtimes, no PDB path")
    print("Private profile paths: none")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("archive", type=Path, default=Path("Grindstone Sword Fix.zip"), nargs="?")
    validate(parser.parse_args().archive)
