# RTOS Security

This module walks through common RTOS security flaws. Shared startup, UART, and network code lives in `common/`. Each numbered directory is one sample.

## Dependencies

```
sudo apt-get update
sudo apt-get install -y gcc-riscv64-unknown-elf qemu-system-misc picolibc-riscv64-unknown-elf cmake
pip install pwntools scapy
```

## Build

```bash
cd misc/rtos
cmake -S . -B build
cmake --build build
```

Run one sample in the foreground, then run that sample's `exploit.py` from another terminal:

```bash
cmake --build build --target run-01-freertos-overflow
cmake --build build --target run-02-executable-stack
```

Only one sample can own `127.0.0.1:8888` at a time.