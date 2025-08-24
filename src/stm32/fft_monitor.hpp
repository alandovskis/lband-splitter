#pragma once
#include "port_state.hpp"
#include <cstdint>

#if defined(__cpp_exceptions)
#error "Exceptions must be disabled for MCU builds"
#endif

struct FFTMonitor {
  static constexpr uint16_t FFT_SIZE = 1024;
  // Processes a block of samples for the given port and updates its
  // frequency measurement. Transmission of status messages is handled in
  // the main loop.
  void process(std::uint8_t port, const float *samples, float sampleRate) noexcept;
};
