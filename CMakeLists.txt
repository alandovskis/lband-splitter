cmake_minimum_required(VERSION 3.10)

project(lband-splitter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SRC CONFIGURE_DEPENDS src/*.cpp)

add_executable(splitter ${SRC})

target_include_directories(splitter PRIVATE src)

target_compile_options(splitter PRIVATE -O2 -Wall)

# Build daemon that receives protobuf messages
find_package(Protobuf REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)
add_executable(monitor_daemon src/daemon/monitor_daemon.cpp src/proto/splitter.pb.cc)
target_include_directories(monitor_daemon PRIVATE src src/proto ${Protobuf_INCLUDE_DIRS})
target_link_libraries(monitor_daemon PRIVATE protobuf::libprotobuf Boost::system Threads::Threads)
target_compile_options(monitor_daemon PRIVATE -O0 -Wall)

enable_testing()

add_executable(crc32_test tests/crc32_test.cpp)
target_include_directories(crc32_test PRIVATE src)
add_test(NAME crc32_test COMMAND crc32_test)

add_executable(center_freq_encoder_test
    tests/center_freq_encoder_test.cpp
    src/stm32/center_freq_encoder.cpp
    src/proto/splitter.pb.cc)
target_include_directories(center_freq_encoder_test PRIVATE src src/stm32 src/proto)
target_link_libraries(center_freq_encoder_test PRIVATE protobuf::libprotobuf)
target_compile_options(center_freq_encoder_test PRIVATE -fno-exceptions)
add_test(NAME center_freq_encoder_test COMMAND center_freq_encoder_test)

add_executable(uart_writer_test tests/uart_writer_test.cpp src/stm32/uart_writer.cpp)
target_include_directories(uart_writer_test PRIVATE src src/stm32)
target_compile_options(uart_writer_test PRIVATE -fno-exceptions)
add_test(NAME uart_writer_test COMMAND uart_writer_test)

add_executable(netconf_server_test tests/netconf_server_test.cpp)
target_include_directories(netconf_server_test PRIVATE src)
target_link_libraries(netconf_server_test PRIVATE Boost::system Threads::Threads)
add_test(NAME netconf_server_test COMMAND netconf_server_test)

add_test(NAME monitor_daemon_report
  COMMAND bash -c "printf 'report { center_mhz: 1.23 } crc32: 3670215876' | protoc -I ${CMAKE_SOURCE_DIR}/proto --encode=splitter.Envelope ${CMAKE_SOURCE_DIR}/proto/splitter.proto | $<TARGET_FILE:monitor_daemon> > /dev/null")
add_test(NAME monitor_daemon_crc_fail
  COMMAND bash -c "printf 'report { center_mhz: 1.23 } crc32: 0' | protoc -I ${CMAKE_SOURCE_DIR}/proto --encode=splitter.Envelope ${CMAKE_SOURCE_DIR}/proto/splitter.proto | $<TARGET_FILE:monitor_daemon> > /dev/null")
set_tests_properties(monitor_daemon_crc_fail PROPERTIES WILL_FAIL TRUE)
