#include "fft_monitor.hpp"

#include "arm_math.h"
#include <cstring>

void FFTMonitor::process(std::uint8_t port, const float *samples,
                         float sampleRate) noexcept {
  float fftInput[FFT_SIZE];
  std::memcpy(fftInput, samples, sizeof(float) * FFT_SIZE);

  arm_rfft_fast_instance_f32 S;
  arm_rfft_fast_init_f32(&S, FFT_SIZE);
  float fftOut[FFT_SIZE];
  arm_rfft_fast_f32(&S, fftInput, fftOut, 0);

  float mags[FFT_SIZE / 2];
  arm_cmplx_mag_f32(fftOut, mags, FFT_SIZE / 2);

  uint32_t index;
  float value;
  arm_max_f32(mags, FFT_SIZE / 2, &value, &index);

  float centerHz = static_cast<float>(index) * sampleRate / FFT_SIZE;
  float centerMHz = centerHz / 1e6f;

  set_port_frequency(port, centerMHz);
}
