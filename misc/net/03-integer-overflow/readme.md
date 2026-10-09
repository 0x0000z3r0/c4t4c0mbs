# Integer Overflow

This lab demonstrates an arithmetic integer overflow. We use `uint16_t` for `inner_len` and calculates `total_len = inner_len + sizeof(struct proxy_header)`. If `inner_len` is very large (e.g., `0xFFFF`), adding the header size (`12`) causes the 16-bit integer to wrap around (`65535 + 12 = 11`). The check `if (total_len > MAX_PACKET_SIZE)` passes because `11 < 256`. We pass `0xFFFF` as `inner_len`. The bounds check is bypassed. The server then copies up to `recv_len` bytes, which is the actual size of our oversized UDP packet, overflowing the 244-byte buffer and overwriting the return address to call `unlock_firmware()`.

## Execution Steps

1. **Compile the proxy:**
  ```bash
   make 03-integer-overflow
   cd 03-integer-overflow
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
  - The proxy will print `inner_len: 65535, total_len: 11`.
  - You should see `========================= FIRMWARE UNLOCKED =========================` in the proxy terminal.

