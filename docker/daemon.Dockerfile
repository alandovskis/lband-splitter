FROM ubuntu:22.04

# Install system dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    python3 \
    python3-pip \
    git \
    && rm -rf /var/lib/apt/lists/*

# Install Conan
RUN pip3 install conan

WORKDIR /app
COPY . .
RUN conan profile detect --force && \
    conan install . --output-folder build --build=missing && \
    cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake && \
    cmake --build build --target monitor_daemon
EXPOSE 9002
CMD ["./build/monitor_daemon"]
