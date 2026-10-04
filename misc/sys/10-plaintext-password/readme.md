# 10: Plaintext password

The password is an ordinary C string. Reverse engineering here is just reading the binary.

Presenter password: `firmware-admin`

## Commands

1. Compile from `misc/sys`:
   ```bash
   make 10-plaintext
   ```
2. Show that the secret is visible without a disassembler:
   ```bash
   strings plaintext-password | grep firmware
   ```
3. Tie that string to the check:
   ```bash
   objdump -d -M intel plaintext-password | sed -n '/<check_password>:/,/^$/p'
   ```
   *`lea` loads the `.rodata` address, then `strcmp` is called. A zero result takes the granted path.*
4. Run it with a wrong password, then with `firmware-admin`.

Next lab removes the literal from `.rodata`.
