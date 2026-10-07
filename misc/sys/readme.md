# Workshop Setup Instructions

This workshop assumes an x86-64 Ubuntu/Debian environment.

## Dependencies

Install the required tools:

```bash
sudo apt update
sudo apt install -y gcc make gdb binutils python3 python3-pip python3-venv checksec
pip install pwntools
```

## ASLR

To ensure the exploits work reliably during the live demonstration, we will disable Address Space Layout Randomization (ASLR) **only for the target process**, rather than disabling it system-wide.

Use the `setarch` command to run a program with ASLR disabled:

```bash
setarch $(uname -m) -R ./vulnerable-binary
```

Or when running exploit scripts:

```bash
setarch $(uname -m) -R python3 exploit.py
```

The exploit scripts provided in this workshop will automatically handle this if they spawn the process using `pwntools` with the correct settings, but it's important to understand the mechanism.

## Mitigations

Throughout the workshop, we will explore the following mitigations:

- **NX (No-eXecute) / DEP:** Prevents executing code on the stack or heap. Bypassed by ROP.
- **Stack Canaries:** Detects stack buffer overflows before the return address is used.
- **PIE (Position Independent Executable) & ASLR:** Randomizes the base address of the binary and libraries, making it hard to hardcode addresses in exploits.
- **RELRO (Relocation Read-Only):** Protects the GOT (Global Offset Table) from being overwritten.
- **FORTIFY_SOURCE:** Replaces certain vulnerable functions (like `strcpy`) with safer variants (`__strcpy_chk`) at compile time if the buffer size is known.

