# Executable stack

FreeRTOS on this RISC-V target does not program a memory-protection unit, and the linker marks RAM writable and executable. The same missing length check as lab 01 overflows a 256-byte stack buffer. This time the return address points back into that buffer, and the bytes from the packet run as code.

`shellcode.S` is a small RV64 program. It polls the QEMU UART and prints a banner. Nothing in the firmware calls it. The packet places those bytes on the task stack, then returns there.

## Run

From `misc/rtos`:

```bash
cmake -S . -B build
cmake --build build --target run-02-executable-stack
```

In another terminal:

```bash
python3 exploit.py
```

The QEMU console prints the shellcode banner after `Payload processed successfully.`