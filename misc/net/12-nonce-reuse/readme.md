# Nonce reuse

Every frame uses an all-zero IV, so AES-128-CTR repeats the keystream. The tag is a CRC of the ciphertext, not an HMAC, so a new ciphertext can be re-tagged.

The user sends the known command `STATUS`. The sniffer XORs it with the captured ciphertext to recover the keystream, then injects `UNLOCK` and a fresh checksum.

## Run

```bash
make 12-nonce-reuse
./proxy unwrap 127.0.0.1 8888 127.0.0.1 9999
```

Second terminal: `./proxy wrap 127.0.0.1 7777 127.0.0.1 8888`

Third terminal: `sudo python3 exploit.py`

Fourth terminal, once the sniffer is waiting:

```bash
printf 'STATUS' | nc -u -w1 127.0.0.1 7777
```

Unwrap ignores `STATUS`, then prints `FIRMWARE UNLOCKED` for the forged command.
