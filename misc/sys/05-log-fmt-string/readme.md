# 05: Format String Vulnerability - Log Service

This lab introduces a completely different class of vulnerability: the Format String bug. It occurs when user input is passed directly to the format string argument of a function like `printf()`.

## Commands to run

1. **Compile:**
  ```bash
   make 05-log-fmt
  ```
2. **Run manually:**
  ```bash
   ./log-service
  ```
   *Try entering* `%p %p %p %p` *to see memory addresses leaked from the stack*
3. **Run Exploit:**
  ```bash
   python3 exploit.py
  ```
   *The script uses the* `%n` *format specifier to write data to an arbitrary memory address, overwriting the* `debug_mode` *variable to spawn a shell.*

