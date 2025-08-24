FROM mcr.microsoft.com/devcontainers/cpp:debian-12

# Install Node.js, npm, and Conan
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get install -y --no-install-recommends nodejs npm \
    && pip3 install --no-cache-dir conan \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace/lband-splitter
