add_executable(splitter_daemon
    main.cpp
    core/splitter_manager.cpp
    core/port.cpp
    core/config_manager.cpp
    hardware/gpio_controller.cpp
    hardware/frequency_detector.cpp
    hardware/led_controller.cpp
    utils/logger.cpp
    utils/system_monitor.cpp
)

target_include_directories(splitter_daemon PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(splitter_daemon
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    pthread
)

install(TARGETS splitter_daemon
    RUNTIME DESTINATION bin
    COMPONENT runtime)