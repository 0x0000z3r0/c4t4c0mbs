# 06: ROP (Return-Oriented Programming) - Web Server

This lab demonstrates how to bypass the NX (No-eXecute) mitigation in a web server scenario. The server itself is too small to contain `pop rdi; ret`, so the chain borrows that gadget, `system`, and the `"/bin/sh"` string from libc. ASLR is disabled for this one process, which makes libc's load address stable and visible in `/proc/<pid>/maps`.

## Commands to run

1. **Compile:**
   ```bash
   make 06-http-rop
   ```
2. **Check Mitigations:**
   ```bash
   checksec --file=web-server
   ```
   *Notice that NX is now enabled. This binary is also built with `-fcf-protection=none`. Ubuntu's default CET shadow stack rejects forged return addresses, which would stop this short chain before it teaches the `pop rdi` idea.*
3. **Find the gadget in libc:**
   ```bash
   python3 -c 'print(hex(open("/lib/x86_64-linux-gnu/libc.so.6","rb").read().find(b"\x5f\xc3")))'
   ```
   *`5f c3` is `pop rdi; ret`. That number is an offset inside the libc file. With ASLR off, add the `libc.so.6` base from `/proc/<pid>/maps`. The exploit does this through `p.libc`.*
4. **Run Exploit:**
   ```bash
   python3 exploit.py
   ```
   *The script builds a ROP chain that pops the address of `/bin/sh` into the `rdi` register (the first argument according to x86-64 calling convention), and then returns to `system()`.*
