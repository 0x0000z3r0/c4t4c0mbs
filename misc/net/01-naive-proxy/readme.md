# Naive Buffer Overflow

This lab demonstrates a classic stack buffer overflow. The proxy blindly trusts the `inner_len` field provided in the network packet's header. `memcpy(payload, buffer + sizeof(struct proxy_header), inner_len);` copies data into a 256-byte stack buffer without checking if `inner_len` is greater than 256. We send a custom UDP packet with an `inner_len` large enough to overwrite the saved return address (RIP) on the stack, redirecting execution to the `unlock_firmware()` function.

## Execution Steps

1. **Compile the proxy:**
  ```bash
   make 01-naive-proxy
   cd 01-naive-proxy
  ```
2. **Run the proxy:**
  ```bash
   ./proxy
  ```
3. **Run the exploit (in another terminal):**
  ```bash
   python3 exploit.py
  ```
4. **Observation:**
  - You should see `========================= FIRMWARE UNLOCKED =========================` in the proxy terminal.

