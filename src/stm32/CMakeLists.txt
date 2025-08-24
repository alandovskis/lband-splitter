cmake_minimum_required(VERSION 3.10)
project(stm32_status_mock LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Protobuf REQUIRED)

add_executable(stm32_status_mock
    status_mock.cpp
    ../proto/splitter.pb.cc
)

add_executable(stm32_client
    client.cpp
    ../proto/splitter.pb.cc
)

# Allow including headers from the parent directory
target_include_directories(stm32_status_mock PRIVATE ..)
target_include_directories(stm32_client PRIVATE ..)

target_compile_options(stm32_status_mock PRIVATE -fno-exceptions)
target_compile_options(stm32_client PRIVATE -fno-exceptions)

target_link_libraries(stm32_status_mock PRIVATE protobuf::libprotobuf pthread)
target_link_libraries(stm32_client PRIVATE protobuf::libprotobuf pthread)
