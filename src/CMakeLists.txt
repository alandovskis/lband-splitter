add_executable(splitter_daemon
    main.cpp
    core/splitter_manager.cpp
    core/port.cpp
    core/config_manager.cpp
    hardware/gpio_controller.cpp
    hardware/stm32f4_controller.cpp
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

# Optional: Build STM32F4 firmware if cross-compilation toolchain is available
find_program(ARM_GCC_COMPILER arm-none-eabi-gcc)
if(ARM_GCC_COMPILER)
    message(STATUS "Found ARM GCC compiler: ${ARM_GCC_COMPILER}")
    message(STATUS "STM32F4 firmware can be built in src/firmware/stm32f4/")
    
    # Add custom target to build STM32F4 firmware
    add_custom_target(stm32f4_firmware
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/firmware
        COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_SOURCE_DIR}/firmware/stm32f4 -B ${CMAKE_BINARY_DIR}/firmware/stm32f4
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/firmware/stm32f4
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/firmware/stm32f4
        COMMENT "Building STM32F4 frequency detector firmware"
    )
    
    # Add firmware files to install
    install(FILES
        ${CMAKE_BINARY_DIR}/firmware/stm32f4/stm32f4_frequency_detector.hex
        ${CMAKE_BINARY_DIR}/firmware/stm32f4/stm32f4_frequency_detector.bin
        DESTINATION share/splitter/firmware
        COMPONENT firmware
        OPTIONAL
    )
else()
    message(STATUS "ARM GCC compiler not found. STM32F4 firmware build will be skipped.")
    message(STATUS "Install gcc-arm-none-eabi to enable STM32F4 firmware compilation.")
endif()