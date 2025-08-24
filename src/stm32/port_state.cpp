#include "port_state.hpp"

PortState g_portStates[NUM_PORTS] = {};

void set_port_enabled(std::size_t port, bool enabled) noexcept {
  if (port < NUM_PORTS) {
    g_portStates[port].enabled = enabled;
  }
}

void set_port_frequency(std::size_t port, float freqMHz) noexcept {
  if (port < NUM_PORTS) {
    g_portStates[port].frequencyMHz = freqMHz;
  }
}

PortState get_port_state(std::size_t port) noexcept {
  if (port < NUM_PORTS) {
    return g_portStates[port];
  }
  return PortState{};
}
