# STM32F4 Firmware Unit Tests

cmake_minimum_required(VERSION 3.15)

# Set up firmware test project
project(stm32f4_firmware_tests C)

# Find Unity testing framework
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(UNITY unity)
endif()

# If Unity not found via pkg-config, use find_path/find_library
if(NOT UNITY_FOUND)
    find_path(UNITY_INCLUDE_DIR unity.h
        HINTS /usr/include /usr/local/include
        PATH_SUFFIXES unity)
    
    find_library(UNITY_LIBRARY
        NAMES unity
        HINTS /usr/lib /usr/local/lib)
    
    if(UNITY_INCLUDE_DIR AND UNITY_LIBRARY)
        set(UNITY_FOUND TRUE)
        set(UNITY_INCLUDE_DIRS ${UNITY_INCLUDE_DIR})
        set(UNITY_LIBRARIES ${UNITY_LIBRARY})
    endif()
endif()

# Include directories
include_directories(
    ${UNITY_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/firmware/stm32f4
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

# Mock STM32 HAL definitions
add_definitions(
    -DHAL_OK=0x00
    -DHAL_ERROR=0x01
    -DHAL_MAX_DELAY=0xFFFFFFFF
    -DGPIO_PIN_SET=1
    -DGPIO_PIN_RESET=0
    -DGPIO_PIN_5=0x0020
    -DGPIO_PIN_6=0x0040
    -DGPIO_PIN_13=0x2000
)

# Create mock STM32 types header
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/stm32f4xx_hal.h "
#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum { HAL_OK = 0x00, HAL_ERROR = 0x01 } HAL_StatusTypeDef;
typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET = 1 } GPIO_PinState;

#define GPIO_PIN_5     0x0020
#define GPIO_PIN_6     0x0040
#define GPIO_PIN_13    0x2000
#define HAL_MAX_DELAY  0xFFFFFFFFU

typedef struct { void* Instance; } UART_HandleTypeDef;
typedef struct { void* Instance; } I2C_HandleTypeDef;
typedef struct { void* Instance; } TIM_HandleTypeDef;
typedef struct { void* Instance; } ADC_HandleTypeDef;
typedef struct { void* Instance; } GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef*)0x40020000)
#define GPIOC ((GPIO_TypeDef*)0x40020800)
#define USART2 ((void*)0x40004400)

// Mock function declarations
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_I2C_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout);
void HAL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
uint32_t HAL_GetTick(void);
")

include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Create frequency detector mock header
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/frequency_detector.h "
#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    double frequency_mhz;
    double snr_db;
    uint16_t raw_frequency;
    uint16_t raw_snr;
    uint32_t timestamp;
    bool valid;
    uint8_t port_id;
} FrequencyReading;

typedef struct {
    bool enabled;
    bool calibrated;
    uint32_t measurement_count;
} FrequencyDetectorState;
")

# Define test executables only if Unity is available
if(UNITY_FOUND)
    message(STATUS "Unity found - building firmware tests")
    
    # Display test executable
    add_executable(test_stm32f4_display
        test_stm32f4_display.c
    )
    target_link_libraries(test_stm32f4_display ${UNITY_LIBRARIES})
    
    # LED test executable  
    add_executable(test_stm32f4_led
        test_stm32f4_led.c
    )
    target_link_libraries(test_stm32f4_led ${UNITY_LIBRARIES})
    
    # UART protocol test executable
    add_executable(test_uart_protocol
        test_uart_protocol.c
    )
    target_link_libraries(test_uart_protocol ${UNITY_LIBRARIES})
    
    # Add tests to CTest
    enable_testing()
    
    add_test(NAME stm32f4_display_tests COMMAND test_stm32f4_display)
    add_test(NAME stm32f4_led_tests COMMAND test_stm32f4_led)  
    add_test(NAME uart_protocol_tests COMMAND test_uart_protocol)
    
    # Set test properties
    set_tests_properties(
        stm32f4_display_tests
        stm32f4_led_tests
        uart_protocol_tests
        PROPERTIES 
            LABELS "firmware"
            TIMEOUT 30
    )
    
else()
    message(WARNING "Unity testing framework not found - skipping firmware tests")
    message(STATUS "To install Unity on Ubuntu/Debian: sudo apt-get install libunity-dev")
    message(STATUS "To install Unity on macOS: brew install unity")
endif()

# Optional: Add custom target for running firmware tests only
if(UNITY_FOUND)
    add_custom_target(test_firmware
        COMMAND ${CMAKE_CTEST_COMMAND} --label-regex firmware --output-on-failure
        DEPENDS test_stm32f4_display test_stm32f4_led test_uart_protocol
        COMMENT "Running STM32F4 firmware unit tests"
    )
endif()