# 07: Heap Use-After-Free (UAF) - Session Manager

This lab introduces heap exploitation. The vulnerability is a Use-After-Free (UAF). The program frees a `Session` struct but later reuses the dangling pointer. In the meantime, the user can allocate a new chunk of the same size, which the heap manager (malloc) will place at the exact same memory address. By writing to this new chunk, we overwrite the function pointer in the old `Session` struct.

## Commands to run

1. **Compile:**
   ```bash
   make 07-session-uaf
   ```
2. **Run manually:**
   ```bash
   ./session-manager
   ```
   *Try entering normal text. It will likely crash because you overwrote the function pointer with invalid ASCII characters.*
3. **Run Exploit:**
   ```bash
   python3 exploit.py
   ```
   *The script allocates the profile data over the freed session and writes the address of the `admin_backdoor()` function precisely where the `greet_func` pointer is expected.*
