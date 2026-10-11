# 01: ELF Basics (System Monitor)

This lab introduces the basic structure of an ELF binary using a realistic scenario. We will see how C structures and global variables translate to x86-64 assembly and ELF sections.

## Commands to run

1. **Compile:**
  ```bash
   make 01-system-monitor
  ```
2. **Inspect Sections:**
  ```bash
   readelf -S system-monitor
  ```
   *Notice* `.text` *(code),* `.data` *(*`global_config`*),* `.bss` *(*`active_connections`*).*
3. **Disassemble:**
  ```bash
   objdump -d -M intel system-monitor | grep -A 20 "<print_system_stats>:"
  ```
   *Notice the prologue (*`push rbp; mov rbp, rsp`*), the argument in* `rdi`*, and the memory addresses for the global structures.*

