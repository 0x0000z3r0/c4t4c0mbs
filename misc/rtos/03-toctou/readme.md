# TOCTOU

The worker checks `flags` on a shared request, waits, then applies that same object. Nothing stops the higher-priority receive task from storing a new packet into it during the wait. The check and the use are not the same read.

The first packet has user flags, so the check passes. The second packet, sent during the wait, sets admin flags. The apply path honors the later value and unlocks the firmware.

## Run

From `misc/rtos`:

```bash
cmake -S . -B build
cmake --build build --target run-03-toctou
```

In another terminal:

```bash
cd 03-toctou
python3 exploit.py
```

The QEMU console shows `check passed, flags=0`, then `applying flags=1`, then `FIRMWARE UNLOCKED`.