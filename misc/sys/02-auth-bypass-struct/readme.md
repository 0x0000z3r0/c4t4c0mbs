# 02: Struct Variable Overwrite (Auth Bypass)

This lab demonstrates a vulnerability where we don't even need to hijack the control flow (no return address overwrite). By overflowing a buffer inside a `struct`, we can overwrite adjacent variables, in this case, an `is_admin` flag.

## Commands to run

1. **Compile:**
  ```bash
   make 02-auth-bypass
  ```
2. **Run manually:**
  ```bash
   ./auth-bypass
  ```
   *Try entering a normal username. Then try entering 33 'A's to see the admin access granted!*
3. **Run Exploit:**
  ```bash
   python3 exploit.py
  ```
   *The script precisely overwrites the 32-byte buffer and sets the* `is_admin` *integer to 1.*

