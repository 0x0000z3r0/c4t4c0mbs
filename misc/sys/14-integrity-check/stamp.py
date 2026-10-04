#!/usr/bin/env python3
"""Fill expected_checksum with the hash of the linked check_password bytes."""

import pathlib
import subprocess
import sys

WINDOW = 64
MARKER = (0xC0FFEE42).to_bytes(4, "little")


def fnv(data):
    value = 2166136261
    for byte in data:
        value ^= byte
        value = (value * 16777619) & 0xFFFFFFFF
    return value


def function_offset(path, name):
    dump = subprocess.check_output(["objdump", "-d", "-F", path], text=True)
    needle = f"<{name}> (File Offset: "
    for line in dump.splitlines():
        if needle in line:
            return int(line.split(needle, 1)[1].split(")", 1)[0], 16)
    raise SystemExit(f"could not find {name}")


def main():
    path = pathlib.Path(sys.argv[1])
    blob = bytearray(path.read_bytes())
    code_at = function_offset(path, "check_password")
    digest = fnv(blob[code_at : code_at + WINDOW]).to_bytes(4, "little")
    found = blob.find(MARKER)
    if found < 0 or blob.find(MARKER, found + 1) >= 0:
        raise SystemExit("expected exactly one checksum marker")
    blob[found : found + 4] = digest
    path.write_bytes(blob)
    print(f"stamped {path} checksum {digest.hex()} at file offset {found:#x}")


if __name__ == "__main__":
    main()
