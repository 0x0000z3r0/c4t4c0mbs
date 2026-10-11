# AES-GCM is not a silver bullet

The tunnel uses AES-128-GCM the way it should be used. The 12-byte nonce is the sequence number, so it is never reused. The tag covers the header and the ciphertext. Unwrap checks the tag before it trusts the bytes, and it rejects a repeated sequence number.

A flipped ciphertext or a replay dies at that check. The crash is after a valid tag. The authenticated plaintext is copied into a 64-byte stack buffer with the authenticated length, so a long genuine command overwrites the return address and calls `unlock_firmware`.

## Run

```bash
make 13-gcm-overflow
./proxy unwrap 127.0.0.1 8888 127.0.0.1 9999
```

Second terminal:

```bash
./proxy wrap 127.0.0.1 7777 127.0.0.1 8888
```

A normal command is sealed and ignored:

```bash
printf 'STATUS' | nc -u -w1 127.0.0.1 7777
```

The overflow is a plaintext datagram. Wrap seals it, unwrap verifies it, then the copy smashes the stack:

```bash
python3 exploit.py
```

Unwrap prints `FIRMWARE UNLOCKED`. Editing the sealed frame on the way to port 8888 makes unwrap print `bad tag` instead.
