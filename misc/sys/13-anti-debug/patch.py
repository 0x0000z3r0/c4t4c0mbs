#!/usr/bin/env python3
"""Turn each debugger check's conditional jump into an unconditional one."""

import pathlib
import sys

from capstone import CS_ARCH_X86, CS_MODE_64, Cs
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

# je/jne +7; mov eax, 1
# tracer_attached uses je (74); ptrace_blocked uses jne (75).
# +7 lands on mov eax, 0, so an unconditional jmp always reports "no debugger".
DETECT_TAILS = (
    bytes.fromhex("74 07 b8 01 00 00 00"),
    bytes.fromhex("75 07 b8 01 00 00 00"),
)


def load_function(path, name):
    """Return (va, file_offset, bytes) for a named ELF symbol."""
    with open(path, "rb") as f:
        elf = ELFFile(f)
        for section in elf.iter_sections():
            if not isinstance(section, SymbolTableSection):
                continue
            for sym in section.iter_symbols():
                if sym.name != name or not sym["st_size"]:
                    continue
                va = sym["st_value"]
                for seg in elf.iter_segments():
                    if seg["p_type"] != "PT_LOAD":
                        continue
                    start = seg["p_vaddr"]
                    if start <= va < start + seg["p_filesz"]:
                        offset = seg["p_offset"] + (va - start)
                        f.seek(offset)
                        return va, offset, f.read(sym["st_size"])
    raise SystemExit(f"could not find {name}")


def dump_function(label, name, va, code):
    print(f"{label}  {name} @ {va:#x}")
    md = Cs(CS_ARCH_X86, CS_MODE_64)
    for insn in md.disasm(code, va):
        mark = ""
        if insn.bytes[:1] in (b"\x74", b"\x75") and insn.bytes[1:2] == b"\x07":
            mark = "  <- patch this opcode to eb (jmp)"
        print(f"  {insn.address:#010x}  {insn.bytes.hex(' '):<20} {insn.mnemonic} {insn.op_str}{mark}")
    print()


def patch_function(blob, path, name):
    va, start, code = load_function(path, name)
    dump_function(str(path), name, va, code)
    at = -1
    for pattern in DETECT_TAILS:
        at = code.find(pattern)
        if at >= 0:
            break
    if at < 0:
        raise SystemExit(f"detect branch not found in {name}")
    blob[start + at] = 0xEB
    dump_function("patched", name, va, bytes(blob[start : start + len(code)]))
    print(f"patched {name} at file offset {start + at:#x}: conditional jump -> jmp\n")


def main():
    src = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "anti-debug")
    dst = pathlib.Path(sys.argv[2] if len(sys.argv) > 2 else "anti-debug.patched")
    blob = bytearray(src.read_bytes())
    patch_function(blob, src, "tracer_attached")
    patch_function(blob, src, "ptrace_blocked")
    dst.write_bytes(blob)
    dst.chmod(0o755)
    print(f"wrote {dst}")


if __name__ == "__main__":
    main()
