#include "PortState.hpp"
#include "SignalProcessor.hpp"
#include <array>
#include <iostream>
#include <cmath>

static constexpr size_t NUM_PORTS = 32;
static constexpr double SAMPLE_RATE = 2e6; // 2 MHz sampling rate

int main() {
    std::array<PortState, NUM_PORTS> ports;

    // Example: generate a 1.55 GHz signal (downconverted to 550 kHz baseband)
    double freqHz = 550e3;
    std::vector<double> samples(1024);
    for (size_t i = 0; i < samples.size(); ++i) {
        samples[i] = std::sin(2 * M_PI * freqHz * i / SAMPLE_RATE);
    }

    double center = computeCenterFreqMHz(samples, SAMPLE_RATE);
    ports[0].enabled = true;
    ports[0].signalPresent = true;
    ports[0].centerFreqMHz = center;

    std::cout << "Port 1 center frequency: " << ports[0].displayText() << std::endl;
    return 0;
}
