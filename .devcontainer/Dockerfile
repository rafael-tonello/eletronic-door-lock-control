FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    python3-venv \
    git \
    curl \
    udev \
    && rm -rf /var/lib/apt/lists/*

# Install PlatformIO Core CLI globally
RUN pip3 install platformio

# Make sure pio is reachable in PATH
ENV PATH="/root/.local/bin:/usr/local/bin:$PATH"
