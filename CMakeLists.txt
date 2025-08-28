cmake_minimum_required(VERSION 3.15)

project(splitter_daemon 
    VERSION 1.0.0
    DESCRIPTION "32-port L-band Splitter/Combiner System"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# find_package(libnetconf2 REQUIRED)  # Not available in ConanCenter
# find_package(cpprest REQUIRED)       # Not available in ConanCenter
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)
# find_package(OpenSSL REQUIRED)       # Build issues on macOS
# find_package(Boost REQUIRED COMPONENTS system thread filesystem)  # Disabled
# find_package(PkgConfig REQUIRED)     # Not needed for simplified build

option(BUILD_TESTING "Build unit tests" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

add_compile_options(-Wall -Wextra -Wpedantic)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
    add_compile_definitions(DEBUG_BUILD)
else()
    add_compile_options(-O2)
endif()

include_directories(src)

add_subdirectory(src)

if(BUILD_TESTING)
    find_package(GTest REQUIRED)
    enable_testing()
    add_subdirectory(tests)
endif()

set(SYSTEMD_UNIT_DIR "/etc/systemd/system" CACHE STRING "Directory for systemd unit files")

install(FILES config/splitter.service
    DESTINATION ${SYSTEMD_UNIT_DIR}
    COMPONENT service)

add_custom_target(install_service
    COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --component service
    COMMAND systemctl daemon-reload
    COMMENT "Installing and reloading systemd service"
    USES_TERMINAL)

# Help target to show available build options
add_custom_target(build_help
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Splitter System Build Targets:"
    COMMAND ${CMAKE_COMMAND} -E echo "  all                 - Build main daemon and tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  splitter_daemon     - Build C++ daemon only"
    COMMAND ${CMAKE_COMMAND} -E echo "  stm32f4_firmware    - Build STM32F4 firmware (requires gcc-arm-none-eabi)"
    COMMAND ${CMAKE_COMMAND} -E echo "  test                - Run unit tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  install             - Install daemon and service files"
    COMMAND ${CMAKE_COMMAND} -E echo "  install_service     - Install systemd service and reload daemon"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "STM32F4 Firmware:"
    COMMAND ${CMAKE_COMMAND} -E echo "  Implements L-band frequency detection (950-2150 MHz)"
    COMMAND ${CMAKE_COMMAND} -E echo "  Compatible with STM32F4 controller protocol"
    COMMAND ${CMAKE_COMMAND} -E echo "  Source: src/firmware/stm32f4/"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMENT "Showing available build targets"
)