# 08: Mitigations

This lab demonstrates how different compiler and OS mitigations affect exploitability, using the firmware update scenario from lab 03.

## Commands to run

1. **Compile:**
   ```bash
   make 08-mitigations
   ```
2. **Check Mitigations:**
   ```bash
   checksec --file=vuln-none
   checksec --file=vuln-nx
   checksec --file=vuln-canary
   checksec --file=vuln-pie
   ```
3. **Observe the effects:**
   - Run `vuln-none` and try a buffer overflow. It will segfault.
   - Run `vuln-nx` and try shellcode. It will segfault (NX blocks execution).
   - Run `vuln-canary` and try a buffer overflow. It will print `*** stack smashing detected ***` and abort.
   - Run `vuln-pie` and use `ldd` or `gdb` to observe that the base address changes every time (unless `setarch -R` is used).
