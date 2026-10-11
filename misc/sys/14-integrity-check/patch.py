#!/usr/bin/env python3
"""Patch the password branch, then optionally update the checksum."""

import pathlib
import sys

from capstone import CS_ARCH_X86, CS_MODE_64, Cs
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

WINDOW = 64
FAIL_BRANCH = bytes.fromhex("75 07 b8 01 00 00 00")
ALWAYS_GRANT = bytes.fromhex("90 90 b8 01 00 00 00")


def fnv(data):
    value = 2166136261
    for byte in data:
        value ^= byte
        value = (value * 16777619) & 0xFFFFFFFF
    return value


def load_symbol(path, name):
    """Return (va, file_offset, size) for a named ELF symbol."""
    with open(path, "rb") as f:
        elf = ELFFile(f)
        for section in elf.iter_sections():
            if not isinstance(section, SymbolTableSection):
                continue
            for sym in section.iter_symbols():
                if sym.name != name or not sym["st_value"]:
                    continue
                va = sym["st_value"]
                size = sym["st_size"]
                for seg in elf.iter_segments():
                    if seg["p_type"] != "PT_LOAD":
                        continue
                    start = seg["p_vaddr"]
                    if start <= va < start + seg["p_filesz"]:
                        return va, seg["p_offset"] + (va - start), size
    raise SystemExit(f"could not find {name}")


def dump_function(label, va, code):
    print(f"{label}  check_password @ {va:#x}")
    md = Cs(CS_ARCH_X86, CS_MODE_64)
    for insn in md.disasm(code, va):
        mark = "  <- patch this jne" if insn.bytes[:1] == b"\x75" else ""
        print(f"  {insn.address:#010x}  {insn.bytes.hex(' '):<20} {insn.mnemonic} {insn.op_str}{mark}")
    print()


def main():
    src = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "integrity-check")
    mode = sys.argv[2] if len(sys.argv) > 2 else "branch"
    dst = pathlib.Path(sys.argv[3] if len(sys.argv) > 3 else f"integrity-check.{mode}")
    if mode not in ("branch", "both"):
        raise SystemExit("mode must be 'branch' or 'both'")

    blob = bytearray(src.read_bytes())
    va, start, size = load_symbol(src, "check_password")
    code = bytes(blob[start : start + size])
    dump_function(str(src), va, code)

    at = code.find(FAIL_BRANCH)
    if at < 0:
        raise SystemExit("password branch not found")
    blob[start + at : start + at + len(ALWAYS_GRANT)] = ALWAYS_GRANT

    if mode == "both":
        digest = fnv(blob[start : start + WINDOW]).to_bytes(4, "little")
        _, slot, _ = load_symbol(src, "expected_checksum")
        blob[slot : slot + 4] = digest
        print(f"updated expected_checksum at {slot:#x} to {digest.hex()}")

    dst.write_bytes(blob)
    dst.chmod(0o755)
    dump_function(str(dst), va, bytes(blob[start : start + size]))
    print(f"wrote {dst}")


if __name__ == "__main__":
    main()
