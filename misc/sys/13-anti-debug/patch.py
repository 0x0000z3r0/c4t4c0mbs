#!/usr/bin/env python3
"""Turn each debugger check's conditional jump into an unconditional one."""

import pathlib
import subprocess
import sys

# je/jne +7; mov eax, 1    ->    jmp +7; mov eax, 1
# `tracer_attached` uses je (74) and `ptrace_blocked` uses jne (75).
# Either way the next instruction is `mov eax, 1`, and +7 lands on `mov eax, 0`.
DETECT_BRANCHES = (
    bytes.fromhex("74 07 b8 01 00 00 00"),
    bytes.fromhex("75 07 b8 01 00 00 00"),
)


def function_offset(path, name):
    dump = subprocess.check_output(["objdump", "-d", "-F", path], text=True)
    needle = f"<{name}> (File Offset: "
    for line in dump.splitlines():
        if needle in line:
            return int(line.split(needle, 1)[1].split(")", 1)[0], 16)
    raise SystemExit(f"could not find {name}")


def patch_function(blob, path, name, limit_name):
    start = function_offset(path, name)
    end = function_offset(path, limit_name)
    window = blob[start:end]
    at = -1
    for branch in DETECT_BRANCHES:
        at = window.find(branch)
        if at >= 0:
            break
    if at < 0:
        raise SystemExit(f"detect branch not found in {name}")
    blob[start + at] = 0xEB
    print(f"patched {name} at file offset {start + at:#x}: conditional jump -> jmp")


def main():
    src = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "anti-debug")
    dst = pathlib.Path(sys.argv[2] if len(sys.argv) > 2 else "anti-debug.patched")
    blob = bytearray(src.read_bytes())
    patch_function(blob, src, "tracer_attached", "ptrace_blocked")
    patch_function(blob, src, "ptrace_blocked", "check_password")
    dst.write_bytes(blob)
    dst.chmod(0o755)
    print(f"wrote {dst}")


if __name__ == "__main__":
    main()
