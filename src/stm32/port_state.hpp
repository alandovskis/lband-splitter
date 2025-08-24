#pragma once

#include <cstddef>
#include <cstdint>

// Total number of RF ports handled by the system
static constexpr std::size_t NUM_PORTS = 32;

struct PortState {
  bool enabled{false};
  float frequencyMHz{0.0f};
};

// Global array holding state for each port
extern PortState g_portStates[NUM_PORTS];

// Sets whether a given port is enabled
void set_port_enabled(std::size_t port, bool enabled) noexcept;

// Updates the measured frequency (in MHz) for a port
void set_port_frequency(std::size_t port, float freqMHz) noexcept;

// Retrieves the current state for a port
PortState get_port_state(std::size_t port) noexcept;
