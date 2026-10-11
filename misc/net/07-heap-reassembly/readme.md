# Heap Reassembly

This lab introduces heap buffer overflows in the context of packet fragmentation and reassembly. The proxy supports fragmented packets. When reassembling, it checks if the total received size (`current_size + inner_len`) exceeds the buffer size. However, it uses the attacker-controlled `frag_offset` to determine *where* to write the data (`memcpy(data + frag_offset, ...)`), without checking if `frag_offset + inner_len` exceeds the buffer bounds. The proxy allocates the data buffer (256 bytes) and a function pointer struct sequentially on the heap. We send a first fragment to initialize the buffer. Then, we send a second fragment with a large `frag_offset` (e.g., 272, skipping over the buffer and heap metadata) to overwrite the function pointer with the address of `unlock_firmware()`. Setting the `LAST` flag triggers the execution of the overwritten pointer.

## Execution Steps

1. **Compile the proxy:**
  ```bash
   make 07-heap-reassembly
   cd 07-heap-reassembly
  ```
2. **Run the proxy:**
  ```bash
   ./proxy
  ```
3. **Run the exploit (in another terminal):**
  ```bash
   python3 exploit.py
  ```
4. **Observation:**
  - The proxy will process the fragments, overwrite the function pointer on the heap, and you should see `========================= FIRMWARE UNLOCKED =========================`.

