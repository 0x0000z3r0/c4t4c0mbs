# RTOS Security

This module walks through common RTOS security flaws. Shared startup, UART, and network code lives in `common/`. Each numbered directory is one sample.

## Dependencies

The `cata-build` image already has the FreeRTOS toolchain: `gcc-riscv64-unknown-elf`, `binutils-riscv64-unknown-elf` (`riscv64-unknown-elf-objdump`), `picolibc-riscv64-unknown-elf`, `qemu-system-misc`, `cmake`, `gdb`, and `pwndbg`.

From the repository root:

```bash
docker build -t cata-build .
docker run --rm -it --name labs -v "$PWD":/labs -w /labs/misc/rtos cata-build
```

The same packages, if you are already on Ubuntu 22.04 and are not using the image:

```
sudo apt-get install -y gcc-riscv64-unknown-elf binutils-riscv64-unknown-elf qemu-system-misc picolibc-riscv64-unknown-elf cmake
pip install pwntools scapy
```

## Build

```bash
cmake -S . -B build
cmake --build build
```

Run one sample in the foreground, then run that sample's `exploit.py` from another terminal:

```bash
cmake --build build --target run-01-freertos-overflow
cmake --build build --target run-02-executable-stack
cmake --build build --target run-03-toctou
```

Only one sample can own `127.0.0.1:8888` at a time.