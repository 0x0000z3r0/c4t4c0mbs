#!/usr/bin/env python3
"""Patch the password branch, then update the checksum so the patch survives."""

import pathlib
import subprocess
import sys

WINDOW = 64
FAIL_BRANCH = bytes.fromhex("75 07 b8 01 00 00 00")
ALWAYS_GRANT = bytes.fromhex("90 90 b8 01 00 00 00")


def fnv(data):
    value = 2166136261
    for byte in data:
        value ^= byte
        value = (value * 16777619) & 0xFFFFFFFF
    return value


def function_offset(path, name):
    dump = subprocess.check_output(["objdump", "-d", "-F", str(path)], text=True)
    needle = f"<{name}> (File Offset: "
    for line in dump.splitlines():
        if needle in line:
            return int(line.split(needle, 1)[1].split(")", 1)[0], 16)
    raise SystemExit(f"could not find {name}")


def symbol_offset(path, name):
    nm = subprocess.check_output(["nm", str(path)], text=True)
    va = None
    for line in nm.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] == name:
            va = int(parts[0], 16)
    if va is None:
        raise SystemExit(f"could not find symbol {name}")
    sections = subprocess.check_output(["readelf", "-S", str(path)], text=True)
    for line in sections.splitlines():
        if ".data" not in line.split():
            continue
        fields = line.split()
        # [Nr] Name Type Address Offset ...
        address = int(fields[3], 16)
        offset = int(fields[4], 16)
        return va - address + offset
    raise SystemExit("could not find .data")


def main():
    src = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "integrity-check")
    mode = sys.argv[2] if len(sys.argv) > 2 else "branch"
    dst = pathlib.Path(sys.argv[3] if len(sys.argv) > 3 else f"integrity-check.{mode}")
    blob = bytearray(src.read_bytes())
    start = function_offset(src, "check_password")
    end = function_offset(src, "code_checksum")
    at = blob[start:end].find(FAIL_BRANCH)
    if at < 0:
        raise SystemExit("password branch not found")
    blob[start + at : start + at + len(ALWAYS_GRANT)] = ALWAYS_GRANT

    if mode == "both":
        digest = fnv(blob[start : start + WINDOW]).to_bytes(4, "little")
        slot = symbol_offset(src, "expected_checksum")
        blob[slot : slot + 4] = digest
        print(f"updated expected_checksum at {slot:#x} to {digest.hex()}")
    elif mode != "branch":
        raise SystemExit("mode must be 'branch' or 'both'")

    dst.write_bytes(blob)
    dst.chmod(0o755)
    print(f"wrote {dst}")


if __name__ == "__main__":
    main()
