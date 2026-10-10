# FreeRTOS Stack Overflow

This lab runs FreeRTOS on a simulated RISC-V 64-bit machine in QEMU. There is no ASLR and no stack canary. `vProxyTask` keeps a 256-byte buffer on the stack and asks `recvfrom` for up to 1024 bytes, so a long UDP payload overwrites the saved return address. The address of `unlock_firmware()` does not change between boots, and returning from the task jumps there.

Network traffic is SLIP over a second UART. QEMU exposes that UART as `127.0.0.1:8888`.

## Run

From `misc/rtos`:

```bash
cmake -S . -B build
cmake --build build --target run-01-freertos-overflow
```

In another terminal:

```bash
python3 exploit.py
```

The QEMU console prints `========================= FIRMWARE UNLOCKED =========================`.
