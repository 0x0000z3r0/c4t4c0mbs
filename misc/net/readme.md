# Network Protocol Security

This is a progression designed to teach network exploitation and secure C programming, focusing on a custom UDP-based packet proxy.

## Protocol Header

The victim program in each lab is `proxy.c`. It acts as a UDP-based packet proxy that receives custom encapsulated packets, parses them, and processes their contents.

```c
struct proxy_header {
    uint32_t magic;      // 0xDEADBEEF
    uint32_t session_id; // Session identifier
    uint16_t inner_len;  // Length of the inner payload
    uint16_t flags;      // Control flags (e.g., INIT, AUTH, DATA, FIN)
};
```

## Dependencies

```shell
sudo apt-get install -y build-essential libssl-dev openssl
pip install pwntools scapy
```

Labs with OpenSSL are more realistic. The program is a UDP tunnel:

```shell
./proxy wrap 127.0.0.1 7777 127.0.0.1 8888
./proxy unwrap 127.0.0.1 8888 127.0.0.1 9999
```

`wrap` accepts plaintext on `127.0.0.1:7777` and sends an encrypted frame to `127.0.0.1:8888`. `unwrap` accepts that frame on `127.0.0.1:8888` and forwards the plaintext to `127.0.0.1:9999`.

## Setup

1. Navigate to the specific lab directory.
2. Run `make` to compile `proxy.c`.
3. Start the proxy server: `./proxy`
4. In another terminal, run the exploit script: `python3 exploit.py`
5. Observe the results and discuss the vulnerability and exploit mechanics.

## Monitoring

Use wireshark to examine the network packets:

```shell
tshark -i lo -f "udp port 8888" -x
```

Optionally, use objdump (or better yet r2/ida) to disassemble important code blocks:

```shell
objdump -S -M intel ./proxy
```

Sending packets manually can be done by using simple CLI tools:

```shell
printf '\xde\xad\xbe\xef\x00\x00\x05\x39\x00\x0c\x00\x00Hello World!' | nc -u -w1 127.0.0.1 8888
```

