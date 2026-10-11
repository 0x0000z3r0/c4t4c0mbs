# 11: Hashed password

Same console as lab 10. The accepted password is no longer a string in the file. `check_password` hashes the input with djb2 (`hash = hash * 33 + byte`, starting at 5381) and compares it with `0xd2f7c478`.

Presenter password, unchanged: `firmware-admin`

## Commands

1. Compile:
   ```bash
   make 11-hashed
   ```
2. Show that `strings` no longer reveals it:
   ```bash
   strings hashed-password | grep firmware || echo 'no plaintext password'
   ```
3. Disassemble the hash and the compare:
   ```bash
   objdump -d -M intel hashed-password | sed -n '/<hash_password>:/,/^$/p'
   objdump -d -M intel hashed-password | sed -n '/<check_password>:/,/^$/p'
   ```
   *Point out the immediate `0xd2f7c478`. Reimplement those few instructions in Python and confirm `firmware-admin` produces it. Say explicitly that this hash is reversible by guessing or brute force; it is not a substitute for a real password hash.*

Next lab skips recovering the password and changes the branch instead.
