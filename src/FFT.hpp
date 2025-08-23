#pragma once
#include <complex>
#include <vector>
#include <cmath>

// Simple recursive Cooley-Tukey FFT for power-of-two sizes.
inline void fft(std::vector<std::complex<double>>& a) {
    const size_t n = a.size();
    if (n <= 1) return;
    std::vector<std::complex<double>> even(n/2), odd(n/2);
    for (size_t i = 0; i < n/2; ++i) {
        even[i] = a[i*2];
        odd[i] = a[i*2+1];
    }
    fft(even);
    fft(odd);
    for (size_t k = 0; k < n/2; ++k) {
        std::complex<double> t = std::polar(1.0, -2*M_PI*k/n) * odd[k];
        a[k] = even[k] + t;
        a[k + n/2] = even[k] - t;
    }
}
