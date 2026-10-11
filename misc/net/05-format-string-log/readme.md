# Format String in Logging

This lab demonstrates a format string vulnerability caused by unsafe logging. We use `printf(msg);` instead of `printf("%s", msg);` inside the `log_error` function. If the `flags` field is set to `0xFFFF`, the proxy treats the packet payload as an error message and passes it to `log_error`. We send a payload containing format string specifiers (e.g., `%x`, `%n`). Using `pwntools`' `fmtstr_payload`, we craft a payload that overwrites the Global Offset Table (GOT) entry for `printf` with the address of `unlock_firmware()`. When `log_error` subsequently calls `printf("\n");`, it actually executes `unlock_firmware()`.

## Execution Steps

1. **Compile the proxy:**
  ```bash
   make 05-format-string-log
   cd 05-format-string-log
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

