# Lab 01: FreeRTOS Stack Overflow

## Dependencies
To compile and run this lab, you need the following packages installed:
```bash
sudo apt-get update
sudo apt-get install -y gcc-riscv64-unknown-elf qemu-system-misc picolibc-riscv64-unknown-elf
pip install pwntools scapy
```
Make sure you have also initialized the FreeRTOS submodule:
```bash
git submodule update --init --recursive
```

## Presenter Notes
- **Concept:** This lab demonstrates how the lack of standard OS mitigations (ASLR, Stack Canaries, MMU) in a typical RTOS environment makes exploitation significantly easier and more reliable.
- **Environment:** We are running FreeRTOS on a simulated RISC-V 32-bit microcontroller using QEMU.
- **Networking:** Because the standard FreeRTOS RISC-V port doesn't include an Ethernet driver, we simulate network traffic by forwarding QEMU's UART (serial port) to a local TCP socket (`127.0.0.1:8888`). Inside FreeRTOS, a hardware interrupt reads bytes from the UART and places them into a Queue, which our `vProxyTask` reads from as if it were a network socket.
- **Vulnerability:** The `vProxyTask` reads a packet header, extracts the `inner_len`, and blindly copies that many bytes into a 256-byte stack buffer.
- **Exploitation:** Because there is no ASLR, the address of `unlock_firmware()` is static and predictable. Because there are no Stack Canaries, we can simply overflow the buffer and overwrite the saved Return Address (`ra` register in RISC-V) on the stack. When the task loop iteration finishes (or if we force a return), it jumps directly to our win function.

## Execution Steps

1. **Compile and Run FreeRTOS in QEMU:**
   ```bash
   cd misc/rtos/01-freertos-overflow
   make run
   ```
   *Note: This will download FreeRTOS, compile the kernel and our custom tasks, and launch QEMU. You will see the FreeRTOS boot message and the proxy task waiting for packets.*

2. **Run the exploit (in another terminal):**
   ```bash
   python3 exploit.py
   ```

3. **Observation:**
   - The Python script connects to the QEMU UART socket and sends the malicious packet.
   - In the QEMU terminal, you will see the proxy task process the payload and then immediately print `========================= FIRMWARE UNLOCKED =========================`.
   - The exploit is 100% reliable every time because the memory layout never changes.