# 04: Shellcode Injection - Sensor Parser

This lab demonstrates how to inject arbitrary machine code (shellcode) into a program's memory and execute it. The scenario is a legacy sensor data parser. This exploit requires the stack to be executable (compiled with `-z execstack`).

## Commands to run

1. **Compile:**
   ```bash
   make 04-sensor
   ```
2. **Check Mitigations:**
   ```bash
   checksec --file=sensor-parser
   ```
   *Notice that NX (No-eXecute) is disabled (NX disabled / RWX).*
3. **Run Exploit:**
   ```bash
   python3 exploit.py
   ```
   *The script injects shellcode, overwrites the return address to point to the shellcode, and gets a shell.*
