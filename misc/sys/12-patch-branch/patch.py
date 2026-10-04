#!/usr/bin/env python3
"""Force check_password to fall through to `return 1`."""

import pathlib
import subprocess
import sys

# jne +7; mov eax, 1    ->    nop; nop; mov eax, 1
FAIL_BRANCH = bytes.fromhex("75 07 b8 01 00 00 00")
ALWAYS_GRANT = bytes.fromhex("90 90 b8 01 00 00 00")


def function_offset(path, name):
    dump = subprocess.check_output(["objdump", "-d", "-F", path], text=True)
    needle = f"<{name}> (File Offset: "
    for line in dump.splitlines():
        if needle in line:
            return int(line.split(needle, 1)[1].split(")", 1)[0], 16)
    raise SystemExit(f"could not find {name}")


def main():
    src = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "patch-branch")
    dst = pathlib.Path(sys.argv[2] if len(sys.argv) > 2 else "patch-branch.patched")
    blob = bytearray(src.read_bytes())
    start = function_offset(src, "check_password")
    end = function_offset(src, "main")
    window = blob[start:end]
    at = window.find(FAIL_BRANCH)
    if at < 0:
        raise SystemExit("password branch not found; rebuild with the lab makefile")
    blob[start + at : start + at + len(ALWAYS_GRANT)] = ALWAYS_GRANT
    dst.write_bytes(blob)
    dst.chmod(0o755)
    print(f"patched {src} -> {dst} at file offset {start + at:#x}")
    print("the conditional jump is now two NOPs, so a wrong password still returns 1")


if __name__ == "__main__":
    main()
