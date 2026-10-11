# 09: Secure Code

This lab demonstrates how to write secure C code to prevent buffer overflows and how to compile it with modern hardening flags.

## Commands to run

1. **Compile:**
   ```bash
   make 09-secure
   ```
2. **Check Mitigations:**
   ```bash
   checksec --file=secure
   ```
   *Notice that all mitigations (NX, PIE, RELRO, Canary, FORTIFY) are enabled.*
3. **Test with normal input:**
   ```bash
   ./secure
   ```
   *Enter a short string.*
4. **Test with oversized input:**
   ```bash
   python3 -c "print('A' * 100)" | ./secure
   ```
   *The program safely truncates the input and does not crash.*
