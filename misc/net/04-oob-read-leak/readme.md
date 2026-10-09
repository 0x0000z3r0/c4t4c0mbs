# Out-of-Bounds Read

This lab demonstrates an Out-of-Bounds (OOB) read vulnerability, similar to the famous Heartbleed bug. We fixed the integer issues and correctly bounds `inner_len` against `MAX_PACKET_SIZE`. However, they fail to verify if the actual received UDP packet (`recv_len`) contains as much data as `inner_len` claims. We send a tiny UDP packet (e.g., just the header and 4 bytes of payload) but set `inner_len = 1000`. The proxy reads 1000 bytes starting from the receive buffer. Because the receive buffer is on the stack and we didn't fill it, the proxy reads past our data into uninitialized stack memory, which contains sensitive data (like `ADMIN_API_KEY`), and echoes it back to us.

## Execution Steps

1. **Compile the proxy:**
  ```bash
   make 04-oob-read-leak
   cd 04-oob-read-leak
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
  - The exploit will print a hex dump of the leaked memory.
  - You should see the `ADMIN_API_KEY` or `SECRET_SESSION_TOKEN` extracted from the leaked stack memory.

