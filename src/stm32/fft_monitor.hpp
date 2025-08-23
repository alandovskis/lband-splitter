#pragma once
#include <cstdint>
#include "uart_writer.hpp"

#if defined(__cpp_exceptions)
#error "Exceptions must be disabled for MCU builds"
#endif

struct FFTMonitor {
    static constexpr uint16_t FFT_SIZE = 1024;
    void process(const float* samples, float sampleRate, UartWriter& uart) noexcept;
};
