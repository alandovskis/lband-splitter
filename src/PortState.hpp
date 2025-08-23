#pragma once
#include <atomic>
#include <string>

struct PortState {
    std::atomic_bool enabled{false};
    std::atomic_bool signalPresent{false};
    std::atomic<double> centerFreqMHz{0.0};

    std::string displayText() const {
        return enabled.load() ?
               std::to_string(centerFreqMHz.load()) + " MHz" :
               "Disabled";
    }
};
