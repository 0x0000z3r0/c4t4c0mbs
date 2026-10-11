# Tag skips the header

The frame carries an HMAC-SHA256, but the HMAC covers only the ciphertext. `flags` sits in the clear header. Unwrap treats `FLAG_ADMIN` as permission to run the command, and that bit is not part of the tag.

## Run

```bash
make 10-tag-skips-header
./proxy unwrap 127.0.0.1 8888 127.0.0.1 9999
```

Second terminal: `./proxy wrap 127.0.0.1 7777 127.0.0.1 8888`

Third terminal: `sudo python3 exploit.py`

Fourth terminal, once the sniffer is waiting:

```bash
printf 'STATUS' | nc -u -w1 127.0.0.1 7777
```

The original datagram has `flags` 0 and is ignored. The exploit injects the same ciphertext with `FLAG_ADMIN` set, and unwrap prints `FIRMWARE UNLOCKED`.
