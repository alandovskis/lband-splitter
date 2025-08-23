#pragma once

#include <cstddef>
#include <cstdint>
#include "stm32f4xx_hal.h"

#if defined(__cpp_exceptions)
#error "Exceptions must be disabled for MCU builds"
#endif

// Simple wrapper around HAL UART transmit
class UartWriter {
public:
    explicit UartWriter(UART_HandleTypeDef* handle) noexcept : handle_(handle) {}

    // Sends the given data over the UART
    void send(const std::uint8_t* data, std::size_t size) const noexcept;

private:
    UART_HandleTypeDef* handle_;
};

