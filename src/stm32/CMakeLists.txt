cmake_minimum_required(VERSION 3.10)
project(stm32_fft_monitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(stm32_fft_monitor
    center_freq_encoder.cpp
    fft_monitor.cpp
    port_state.cpp
    uart_writer.cpp
    hal_stubs.cpp
    ../proto/splitter.pb.cc
)

target_include_directories(stm32_fft_monitor PUBLIC . ../proto ..)

target_compile_options(stm32_fft_monitor PRIVATE -fno-exceptions)
