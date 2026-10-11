# Signedness Bypass

his lab demonstrates how integer signedness issues can bypass bounds checks. We have added a check `if (inner_len > MAX_PAYLOAD)` but parsed `inner_len` as a signed 16-bit integer (`int16_t`). Furthermore, the safe copy check `recv_len - sizeof(struct proxy_header) < bytes_to_copy` contains an implicit cast to an unsigned type because `sizeof` returns `size_t`. We pass `-1` (`0xFFFF`) as `inner_len`. `-1` is not greater than 256, so it passes the first check. In the second check, `-1` is cast to a massive unsigned number, making the condition true. `bytes_to_copy` is then set to the actual received payload length, which allows us to copy our oversized payload and overwrite the stack, redirecting execution to `unlock_firmware()`.

## Execution Steps

1. **Compile the proxy:**
  ```bash
   make 02-signedness-bypass
   cd 02-signedness-bypass
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

