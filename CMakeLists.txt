cmake_minimum_required(VERSION 3.16)
project(LBandSplitter VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Find libraries
pkg_check_modules(JSONCPP REQUIRED jsoncpp)
pkg_check_modules(LIBYANG REQUIRED libyang)
pkg_check_modules(LIBNETCONF2 REQUIRED libnetconf2)

# Include directories
include_directories(src)
include_directories(${JSONCPP_INCLUDE_DIRS})
include_directories(${LIBYANG_INCLUDE_DIRS})
include_directories(${LIBNETCONF2_INCLUDE_DIRS})

# Hardware abstraction layer library
set(HARDWARE_SOURCES
    src/hardware/gpio_controller.cpp
    src/hardware/frequency_detector.cpp
)
add_library(splitter_hardware STATIC ${HARDWARE_SOURCES})
target_link_libraries(splitter_hardware Threads::Threads)

# Core splitter logic library
set(CORE_SOURCES
    src/core/splitter_port.cpp
    src/core/splitter_system.cpp
)
add_library(splitter_core STATIC ${CORE_SOURCES})
target_link_libraries(splitter_core splitter_hardware Threads::Threads)

# NetConf interface library
set(NETCONF_SOURCES
    src/netconf/netconf_server.cpp
)
add_library(splitter_netconf STATIC ${NETCONF_SOURCES})
target_link_libraries(splitter_netconf 
    splitter_core 
    ${LIBYANG_LIBRARIES}
    ${LIBNETCONF2_LIBRARIES}
    Threads::Threads
)

# Web REST API library
set(WEB_SOURCES
    src/web/rest_server.cpp
)
add_library(splitter_web STATIC ${WEB_SOURCES})
target_link_libraries(splitter_web 
    splitter_core 
    ${JSONCPP_LIBRARIES}
    Threads::Threads
)

# Main executable
add_executable(splitter src/main.cpp)
target_link_libraries(splitter 
    splitter_hardware
    splitter_core
    splitter_netconf
    splitter_web
    ${JSONCPP_LIBRARIES}
    ${LIBYANG_LIBRARIES}
    ${LIBNETCONF2_LIBRARIES}
    Threads::Threads
)

# Installation rules
install(TARGETS splitter DESTINATION bin)
install(FILES config/splitter.conf DESTINATION /etc/splitter)
install(FILES config/yang/splitter-system.yang DESTINATION /usr/local/share/yang/modules)

# Custom targets for Angular webapp
add_custom_target(webapp
    COMMAND npm install
    COMMAND npm run build
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/webapp
    COMMENT "Building Angular webapp"
)

# Install webapp
add_custom_target(install_webapp
    COMMAND ${CMAKE_COMMAND} -E make_directory /var/www/splitter
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/webapp/dist /var/www/splitter
    DEPENDS webapp
    COMMENT "Installing webapp to /var/www/splitter"
)

# Development target
add_custom_target(dev
    COMMAND sudo $<TARGET_FILE:splitter> --config ${CMAKE_SOURCE_DIR}/config/splitter.conf --debug
    DEPENDS splitter
    COMMENT "Running splitter in development mode"
)

# Testing
enable_testing()

# Find Google Test
find_package(GTest QUIET)
if(GTest_FOUND)
    # Unit tests
    set(TEST_SOURCES
        tests/unit/test_gpio_controller.cpp
        tests/unit/test_frequency_detector.cpp
        tests/unit/test_splitter_port.cpp
        tests/unit/test_splitter_system.cpp
    )
    
    add_executable(test_splitter ${TEST_SOURCES})
    target_link_libraries(test_splitter 
        splitter_hardware
        splitter_core
        GTest::gtest_main
        Threads::Threads
    )
    
    add_test(NAME unit_tests COMMAND test_splitter)
    
    # Custom test target
    add_custom_target(test_all
        COMMAND ${CMAKE_CTEST_COMMAND} --verbose
        DEPENDS test_splitter
        COMMENT "Running all tests"
    )
else()
    message(WARNING "Google Test not found. Tests will not be built.")
endif()

# System service installation
add_custom_target(install_service
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/scripts/systemd/splitter.service /etc/systemd/system/
    COMMAND systemctl daemon-reload
    COMMENT "Installing systemd service"
)

# Full installation target
add_custom_target(install_all
    DEPENDS install install_webapp install_service
    COMMENT "Complete installation"
)

# Print build information
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")

# Package configuration
set(CPACK_PACKAGE_NAME "lband-splitter")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "32-port L-Band Splitter/Combiner Control System")
set(CPACK_PACKAGE_VENDOR "L-Band Splitter Project")
set(CPACK_PACKAGE_CONTACT "admin@lband-splitter.local")

set(CPACK_GENERATOR "DEB;RPM;TGZ")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libyang2, libnetconf2-2, libjsoncpp25")
set(CPACK_RPM_PACKAGE_REQUIRES "libyang >= 2.0, libnetconf2 >= 2.0, jsoncpp >= 1.9")

include(CPack)