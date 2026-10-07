# 13: Anti-debug checks

Two checks run before the password is read:

- `tracer_attached` reads `TracerPid` from `/proc/self/status`. A non-zero pid means something, usually a debugger, is attached.
- `ptrace_blocked` calls `ptrace(PTRACE_TRACEME)`. That call fails when a debugger is already attached.

Each function ends in a short conditional jump over `mov eax, 1`:

```text
tracer_attached:  je   not_traced     ; 74 07
ptrace_blocked:   jne  not_traced     ; 75 07
                  mov  eax, 1         ; debugger detected
```

Changing that opcode to `eb` (`jmp`) skips the failure return whether or not a debugger is present. `patch.py` does this in both functions.

`PTRACE_TRACEME` has a side effect on a normal run: after it succeeds, a later attach from another debugger is rejected. Run the password demo before attaching GDB, or use the patched binary for the debugger demo.

Presenter password: `firmware-admin`

## Commands

1. Compile:
   ```bash
   make 13-anti-debug
   ```
2. Normal run, then the same binary under GDB:
   ```bash
   printf 'firmware-admin\n' | ./anti-debug
   gdb -batch -ex 'set pagination off' -ex 'run' -ex 'quit' --args ./anti-debug <<< 'firmware-admin'
   ```
   *The second command prints `Debugger detected` and never grants access.*
3. Patch both checks. `patch.py` prints each function before and after:
   ```bash
   python3 patch.py anti-debug anti-debug.patched
   gdb -batch -ex 'set pagination off' -ex 'run' -ex 'quit' --args ./anti-debug.patched <<< 'firmware-admin'
   ```

Next lab checksums the password function, so this kind of byte patch is noticed.
