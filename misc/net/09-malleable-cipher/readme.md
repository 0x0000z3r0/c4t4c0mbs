# Encryption without a tag

`proxy wrap` encrypts whatever UDP payload it receives with AES-128-CTR and sends it on. `proxy unwrap` decrypts and trusts the result. There is no tag, so a sniffer can flip ciphertext bits and change `STATUS` into `UNLOCK` without the key.

## Run

```bash
make 09-malleable-cipher
./proxy unwrap 127.0.0.1 8888 127.0.0.1 9999
```

Second terminal:

```bash
./proxy wrap 127.0.0.1 7777 127.0.0.1 8888
```

Third terminal:

```bash
python3 exploit.py
# sudo env PYTHONPATH=/home/$USER/.local/lib/python3.10/site-packages python3 exploit.py
```

Fourth terminal, once the sniffer is waiting. `printf` sends exactly six bytes; `echo` would append a newline.

```bash
printf 'STATUS' | nc -u -w1 127.0.0.1 7777
```

Unwrap prints `command: STATUS` for the original datagram, then `command: UNLOCK` and `FIRMWARE UNLOCKED` for the injected one.

Watch the encrypted hop with `tshark -i lo -f "udp port 8888" -x`.