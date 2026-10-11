# Replay

The HMAC covers the header, IV, and ciphertext, including `seq`. Unwrap checks the tag and then forgets the sequence number, so a captured datagram can be sent again.

## Run

```bash
make 11-replay
./proxy unwrap 127.0.0.1 8888 127.0.0.1 9999
```

Second terminal: `./proxy wrap 127.0.0.1 7777 127.0.0.1 8888`

Third terminal: `sudo python3 exploit.py`

Fourth terminal, once the sniffer is waiting:

```bash
printf 'UNLOCK' | nc -u -w1 127.0.0.1 7777
```

Unwrap prints `FIRMWARE UNLOCKED` for the original datagram, then again for the replayed copy.
