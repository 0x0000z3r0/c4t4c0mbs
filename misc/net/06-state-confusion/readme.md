# State Machine Confusion

This lab demonstrates logic flaws in stateful network protocols. The proxy implements a simple state machine (`INIT` -> `AUTH` -> `DATA`). However, when processing a `DATA` packet, it checks if the global `current_session` is authenticated, but fails to verify if the incoming packet's `session_id` matches the authenticated session's ID. If any legitimate user is currently authenticated on the server, an attacker can send a `DATA` packet with their own session ID. The server sees that the state is `AUTHED` and processes the attacker's data, bypassing the authentication requirement.

## Execution Steps
1. **Compile the proxy:**
   ```bash
   make 06-state-confusion
   cd 06-state-confusion
   ```
2. **Run the proxy:**
   ```bash
   ./proxy
   ```
   *(Notice that it simulates a legitimate user already being authenticated).*
3. **Run the exploit (in another terminal):**
   ```bash
   python3 exploit.py
   ```
4. **Observation:**
   - The proxy will accept the `DATA` packet despite the session ID mismatch.
   - You should see `[!] Root command received!` followed by `========================= FIRMWARE UNLOCKED =========================` in the proxy terminal.