# 12: Patch the branch

The password is still `firmware-admin`, but this lab does not recover it. `check_password` is written as an if/else so `-O0` keeps a real conditional jump:

```text
test   eax, eax
jne    fail          ; 75 07
mov    eax, 1        ; password matched
jmp    done
fail:
mov    eax, 0
```

`strcmp` returns 0 on a match, so `jne` jumps to the failure return whenever the password is wrong. Replacing `75 07` with `90 90` deletes that jump. Execution falls into `mov eax, 1` for every input.

## Commands

1. Compile:
   ```bash
   make 12-patch-branch
   ```
2. Confirm a wrong password is rejected, then patch. `patch.py` prints `check_password` before and after:
   ```bash
   printf 'nope\n' | ./patch-branch
   python3 patch.py patch-branch patch-branch.patched
   printf 'nope\n' | ./patch-branch.patched
   ```

Next lab adds checks that refuse to run while a debugger is attached. Those checks are patched the same way.
