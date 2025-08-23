# L-Band Splitter Controller

This directory contains sample C++ code and an Angular component stub for a 32-port L-band splitter/combiner.

The C++ demo computes the center frequency of a synthetic signal using an FFT and stores the result in a `PortState` object.

## Prerequisites

The project uses Conan to provide the required third-party packages such as
Protobuf and Boost.  Install Conan (``pip install conan`` if needed) and let it
detect your host profile:

```
conan profile detect --force
```

## Build and run

Install dependencies and build using CMake:

```
conan install . --output-folder build --build=missing
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake
cmake --build build
./build/splitter
```

Run unit tests with CTest:

```
cd build && ctest
```

The Angular component (`web/port-status.component.*`) illustrates how a frontend might interact with the backend.

An example Nginx configuration in `web/nginx.conf` shows how to serve the
compiled Angular assets while terminating TLS and proxying `/ws` WebSocket
requests to the daemon running on port 9002.

## STM32 microcontroller implementation

An embedded variant places the FFT and monitoring on an STM32 MCU using the CMSIS-DSP library. Example code in `src/stm32/fft_monitor.cpp` computes the center frequency of a sample buffer and sends the result over UART.

The embedded build forbids C++ exceptions. A standalone CMake project in `src/stm32` builds the MCU code and compiles with `-fno-exceptions`.

## Protobuf protocol and daemon

A lightweight protocol defined in `proto/splitter.proto` allows the microcontroller to send FFT reports or receive commands such as start/stop and enabling or disabling individual ports. Each envelope includes a CRC32 checksum for basic integrity checking. A simple daemon (`monitor_daemon`) parses these protobuf messages on the Linux SBC, verifies the checksum, and prints the results. The daemon also launches a WebSocket server for communication with the Angular frontend and exposes a NETCONF interface for external control and monitoring, enabling remote clients to toggle ports and query their detected center frequencies.
