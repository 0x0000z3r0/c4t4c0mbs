# 03: Stack Overflow (ret2win) - Firmware Update

This lab demonstrates a classic stack buffer overflow in a realistic scenario: an IoT device firmware updater. We will overwrite the saved return address on the stack to redirect execution to a hidden `admin_debug_shell()` function left by the developers.

Unlike previous labs, the program does not print the address of the hidden function for us. We must reverse engineer the binary to find it!

## Commands to run

1. **Compile:**
   ```bash
   make 03-firmware
   ```
2. **Reverse Engineer (Find the Backdoor):**
   Use `objdump` to list the functions and find the address of the backdoor:
   ```bash
   objdump -d -M intel firmware-update | grep "<admin_debug_shell>:"
   ```
   *Note the hexadecimal address printed on the left. Because PIE (Position Independent Executable) is disabled for this lab, this address is static and will not change between runs.*
3. **Run manually:**
   ```bash
   ./firmware-update
   ```
   *Try entering a short string, then try a very long string (e.g., 100 'A's) to see the segmentation fault.*
4. **Run Exploit:**
   ```bash
   python3 exploit.py
   ```
   *The script uses `pwntools` to automatically extract the symbol address from the ELF file, calculates the offset, constructs the payload, and drops you into a shell.*
