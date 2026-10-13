# System Security

This workshop assumes x86-64 Ubuntu 22.04. Several `exploit.py` files hardcode stack offsets from GCC 11. Ubuntu 24's GCC lays those frames out differently, so a host build there will not match the scripts.

From the repository root:

```bash
docker build -t cata-build .
docker run --rm -it --name labs -v "$PWD":/labs -w /labs/misc/sys cata-build
```

A second shell joins the same container:

```bash
docker exec -it labs bash
```

Inside the container, `make` uses that image's GCC. The image already has `gcc`, `make`, `gdb`, `pwndbg`, `binutils` (`objdump`), `checksec`, `pwntools`, `tshark`, and `tmux`.

## Dependencies

The same packages, if you are already on Ubuntu 22.04 and are not using the image:

```bash
sudo apt update
sudo apt install -y gcc make gdb binutils python3 python3-pip python3-venv checksec
pip install pwntools pyelftools capstone
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

