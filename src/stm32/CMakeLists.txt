cmake_minimum_required(VERSION 3.10)
project(stm32_status_mock LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(stm32_status_mock
    status_mock.cpp
)

# Allow including headers from the parent directory
target_include_directories(stm32_status_mock PRIVATE ..)

target_compile_options(stm32_status_mock PRIVATE -fno-exceptions)
