#include "SignalProcessor.hpp"
#include "FFT.hpp"
#include <complex>
#include <cmath>
#include <algorithm>

// Compute center frequency in MHz using a basic FFT peak search.
double computeCenterFreqMHz(const std::vector<double>& samples, double sampleRate) {
    size_t n = samples.size();
    // pad to next power of two
    size_t m = 1;
    while (m < n) m <<= 1;
    std::vector<std::complex<double>> data(m);
    for (size_t i = 0; i < n; ++i) data[i] = samples[i];
    fft(data);
    // Find peak magnitude
    size_t peakIndex = 0;
    double peakMag = 0.0;
    for (size_t i = 1; i < m/2; ++i) {
        double mag = std::abs(data[i]);
        if (mag > peakMag) {
            peakMag = mag;
            peakIndex = i;
        }
    }
    double freq = (double)peakIndex * sampleRate / m;
    return freq / 1e6; // convert Hz to MHz
}
