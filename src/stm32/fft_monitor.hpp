#pragma once
#include "port_state.hpp"
#include "uart_writer.hpp"
#include <cstdint>

#if defined(__cpp_exceptions)
#error "Exceptions must be disabled for MCU builds"
#endif

struct FFTMonitor {
  static constexpr uint16_t FFT_SIZE = 1024;
  // Processes a block of samples for the given port, updating its
  // frequency measurement and emitting a center frequency report.
  void process(std::uint8_t port, const float *samples, float sampleRate,
               UartWriter &uart) noexcept;
};
