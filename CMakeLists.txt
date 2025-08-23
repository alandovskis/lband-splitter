cmake_minimum_required(VERSION 3.10)

project(lband-splitter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SRC CONFIGURE_DEPENDS src/*.cpp)

add_executable(splitter ${SRC})

target_include_directories(splitter PRIVATE src)

target_compile_options(splitter PRIVATE -O2 -Wall)
