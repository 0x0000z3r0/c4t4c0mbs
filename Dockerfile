FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN echo 'wireshark-common wireshark-common/install-setuid boolean false' | debconf-set-selections \
    && apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        gcc \
        make \
        gdb \
        binutils \
        libssl-dev \
        python3 \
        python3-pip \
        python3-venv \
        checksec \
        netcat-openbsd \
        ca-certificates \
        curl \
        tmux \
        tshark \
        cmake \
        gcc-riscv64-unknown-elf \
        binutils-riscv64-unknown-elf \
        picolibc-riscv64-unknown-elf \
        qemu-system-misc \
    && pip3 install --no-cache-dir pwntools scapy pyelftools capstone \
    && rm -rf /var/lib/apt/lists/* \
    && curl --proto '=https' --tlsv1.2 -LsSf 'https://install.pwndbg.re' | sh -s -- -t pwndbg-gdb

WORKDIR /labs
