# 14: Integrity check

`check_password` is the same branch as lab 12. Before asking for a password, `main` hashes the first 64 bytes at `check_password` and compares the result with `expected_checksum`.

The source initializes that global to the marker `0xc0ffee42`. `make` runs `stamp.py` after linking and replaces the marker with the hash of the bytes just linked. A patch of the `jne` inside those 64 bytes changes the hash.

## Commands

1. Compile, which also stamps the checksum:
  ```bash
   make 14-integrity
  ```
2. Wrong password is denied, and the program still starts:
  ```bash
   printf 'nope\n' | ./integrity-check
  ```
3. Patch only the branch. `patch.py` prints `check_password` before and after. Startup now fails:
  ```bash
   python3 patch.py integrity-check branch integrity-check.branch
   ./integrity-check.branch
  ```
4. Patch the branch and rewrite `expected_checksum` with the new hash:
  ```bash
   python3 patch.py integrity-check both integrity-check.both
   printf 'nope\n' | ./integrity-check.both
  ```
   `both` *grants access. The checksum did not protect the policy; it only moved the patch from one compare to two.*

This is the end of the reversing sequence. A serious integrity check is signed and verified before the patched code runs; a constant sitting next to the code can be edited with it.