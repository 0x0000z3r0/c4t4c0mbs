# Lab 08: Secure Proxy

## **Mitigations Implemented:**

- **Buffer Overflows:** Uses `MAX_PACKET_SIZE` to bound the buffer and performs strict length checks.
- **Integer Issues:** Uses unsigned integers (`uint16_t`) for lengths to prevent signedness bypasses. Avoids arithmetic overflows by checking `inner_len` against `MAX_PACKET_SIZE - sizeof(header)`.
- **Out-of-Bounds Reads:** Verifies that the claimed `inner_len` does not exceed the actual received data (`recv_len - sizeof(header)`).
- **Format Strings:** Uses `printf("%s", msg)` instead of passing user input directly as the format string.
- **State Confusion:** Enforces strict state transitions and validates that the incoming packet's `session_id` matches the currently authenticated session.
- **Compiler Mitigations:** Compiled with Stack Canaries (`-fstack-protector-all`), Position Independent Executable (`-pie -fPIE`), and full RELRO (`-Wl,-z,relro,-z,now`), making exploitation significantly harder even if a vulnerability existed.

## Execution Steps

1. **Compile the proxy:**
  ```bash
   make 08-secure-proxy
   cd 08-secure-proxy
  ```
2. **Run the proxy:**
  ```bash
   ./proxy
  ```

