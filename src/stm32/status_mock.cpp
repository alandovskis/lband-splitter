#include "PortState.hpp"
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

// Simple STM32 mock that toggles port states and prints changes.
int main() {
    constexpr std::size_t NUM_PORTS = 4;
    std::vector<PortState> ports(NUM_PORTS);

    for (;;) {
        for (std::size_t i = 0; i < NUM_PORTS; ++i) {
            bool newEnabled = !ports[i].enabled.load();
            ports[i].enabled = newEnabled;
            ports[i].signalPresent = newEnabled;
            ports[i].centerFreqMHz = newEnabled ? 1000.0 + static_cast<double>(i) : 0.0;
            std::cout << "Port " << i
                      << (newEnabled ? " enabled" : " disabled")
                      << ", frequency: " << ports[i].centerFreqMHz.load() << " MHz"
                      << ", signal: " << (ports[i].signalPresent.load() ? "present" : "absent")
                      << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    return 0;
}
