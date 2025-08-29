find_package(GTest REQUIRED)

include(GoogleTest)

add_executable(splitter_tests
    unit/test_config_manager.cpp
    unit/test_logger.cpp
    unit/test_stm32f4_controller.cpp
    # Disabled - need mock fixes
    # unit/test_splitter_manager.cpp
    # unit/test_gpio_controller.cpp
    # unit/test_frequency_detector.cpp
    # unit/test_led_controller.cpp
    # unit/test_system_monitor.cpp
    # Disabled - missing dependencies
    # unit/test_port.cpp
    # unit/test_netconf_server.cpp
    # unit/test_yang_model.cpp
    # unit/test_rest_server.cpp
    # unit/test_websocket_server.cpp
    # integration/test_full_system.cpp
    # mocks/mock_gpio_controller.cpp
    # mocks/mock_hardware.cpp
)

target_include_directories(splitter_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(splitter_tests
    GTest::gtest
    GTest::gtest_main
    GTest::gmock
    GTest::gmock_main
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    pthread
)

add_library(splitter_test_lib STATIC
    ../src/core/port.cpp
    ../src/core/splitter_manager.cpp
    ../src/core/config_manager.cpp
    ../src/hardware/stm32f4_controller.cpp
    ../src/utils/logger.cpp
    ../src/utils/system_monitor.cpp
)

target_include_directories(splitter_test_lib PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(splitter_test_lib
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    pthread
)

target_link_libraries(splitter_tests splitter_test_lib)

gtest_discover_tests(splitter_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)

add_test(NAME splitter_unit_tests COMMAND splitter_tests)

set_tests_properties(splitter_unit_tests PROPERTIES
    TIMEOUT 300
    LABELS "unit"
)