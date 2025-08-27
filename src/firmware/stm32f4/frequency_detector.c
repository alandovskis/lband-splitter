#include "frequency_detector.h"
#include "stm32f4xx_hal.h"
#include <math.h>
#include <string.h>

// External ADC handle (defined in main.c)
extern ADC_HandleTypeDef hadc1;

// Simple FFT implementation for STM32F4
// Using a basic radix-2 decimation-in-time FFT
static void fft_radix2(float *real, float *imag, uint16_t size);
static void bit_reverse(float *data, uint16_t size);

void frequency_detector_init(FrequencyDetectorState *state) {
  memset(state, 0, sizeof(FrequencyDetectorState));
  
  state->sample_index = 0;
  state->buffer_full = false;
  state->history_index = 0;
  state->current_frequency_mhz = 0.0f;
  state->noise_floor_dbm = NOISE_FLOOR_DBM;
  state->signal_power_dbm = MIN_SIGNAL_LEVEL_DBM;
  state->snr_db = MIN_SNR_DB;
  
  // Initialize calibration with default values
  state->frequency_calibration_offset = 0.0f;
  state->amplitude_calibration_factor = 1.0f;
  state->calibrated = false;
  
  state->enabled = true;
  state->measurement_count = 0;
  state->last_measurement_time = HAL_GetTick();
}

bool frequency_detector_measure(FrequencyDetectorState *state, FrequencyReading *reading) {
  if (!state->enabled || !state->buffer_full) {
    return false;
  }
  
  // Process FFT on the collected samples
  process_fft(state);
  
  // Detect peak frequency
  float detected_freq = detect_peak_frequency(state->magnitude_spectrum, FFT_SIZE / 2);
  
  if (detected_freq > 0.0f) {
    // Apply frequency calibration
    detected_freq += state->frequency_calibration_offset;
    
    // Update frequency history for stability checking
    state->frequency_history[state->history_index] = detected_freq;
    state->history_index = (state->history_index + 1) % FREQUENCY_STABILITY_SAMPLES;
    
    // Check if frequency is stable
    if (is_frequency_stable(state)) {
      state->current_frequency_mhz = detected_freq;
    }
    
    // Calculate SNR
    uint16_t peak_bin = (uint16_t)((detected_freq - FREQ_MIN_MHZ) / 
                                   ((FREQ_MAX_MHZ - FREQ_MIN_MHZ) / (FFT_SIZE / 2)));
    state->snr_db = calculate_snr(state->magnitude_spectrum, peak_bin, FFT_SIZE / 2);
    
    // Prepare reading
    reading->frequency_mhz = state->current_frequency_mhz;
    reading->snr_db = state->snr_db;
    reading->signal_level_dbm = state->signal_power_dbm;
    reading->raw_frequency = frequency_to_raw(state->current_frequency_mhz);
    reading->raw_snr = (uint16_t)((state->snr_db - MIN_SNR_DB) / 
                                  (MAX_SNR_DB - MIN_SNR_DB) * 65535);
    reading->timestamp = HAL_GetTick();
    reading->valid = (state->current_frequency_mhz >= FREQ_MIN_MHZ && 
                     state->current_frequency_mhz <= FREQ_MAX_MHZ &&
                     state->snr_db > (NOISE_FLOOR_DBM + PEAK_DETECTION_THRESHOLD));
    reading->port_id = state->port_id;
    
    state->measurement_count++;
    state->last_measurement_time = HAL_GetTick();
    
    return true;
  }
  
  return false;
}

void frequency_detector_adc_callback(FrequencyDetectorState *state, uint16_t adc_value) {
  if (!state->enabled) {
    return;
  }
  
  // Store ADC sample in circular buffer
  state->sample_buffer[state->sample_index] = adc_value;
  state->sample_index = (state->sample_index + 1) % SAMPLE_BUFFER_SIZE;
  
  if (state->sample_index == 0) {
    state->buffer_full = true;
  }
  
  // Update signal power estimation
  state->signal_power_dbm = dbm_from_adc(adc_value);
}

bool frequency_detector_calibrate(FrequencyDetectorState *state) {
  // Perform calibration measurement with known reference signal
  // This would typically involve injecting a known frequency and measuring the response
  
  uint32_t start_time = HAL_GetTick();
  uint32_t calibration_samples = 0;
  float frequency_sum = 0.0f;
  
  // Reset buffer for calibration
  state->buffer_full = false;
  state->sample_index = 0;
  
  // Collect calibration data for 1 second
  while ((HAL_GetTick() - start_time) < 1000) {
    if (state->buffer_full) {
      FrequencyReading reading;
      if (frequency_detector_measure(state, &reading)) {
        if (reading.valid) {
          frequency_sum += reading.frequency_mhz;
          calibration_samples++;
        }
      }
      state->buffer_full = false;
      state->sample_index = 0;
    }
    HAL_Delay(10);
  }
  
  if (calibration_samples > 10) {
    float measured_freq = frequency_sum / calibration_samples;
    
    // Assume we're calibrating with a 1575.42 MHz GPS L1 signal
    float reference_freq = 1575.42f;
    state->frequency_calibration_offset = reference_freq - measured_freq;
    state->calibrated = true;
    
    return true;
  }
  
  return false;
}

void frequency_detector_reset(FrequencyDetectorState *state) {
  state->sample_index = 0;
  state->buffer_full = false;
  state->history_index = 0;
  state->current_frequency_mhz = 0.0f;
  state->measurement_count = 0;
  
  memset(state->sample_buffer, 0, sizeof(state->sample_buffer));
  memset(state->frequency_history, 0, sizeof(state->frequency_history));
}

static void process_fft(FrequencyDetectorState *state) {
  // Copy ADC samples to FFT input buffer and apply window function
  for (uint16_t i = 0; i < FFT_SIZE; i++) {
    // Hann window
    float window = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (FFT_SIZE - 1)));
    state->fft_input[i] = (float)(state->sample_buffer[i] - 2048) * window / 2048.0f;
  }
  
  // Perform FFT (simplified implementation for demonstration)
  memset(state->fft_output, 0, sizeof(state->fft_output));
  fft_radix2(state->fft_input, state->fft_output, FFT_SIZE);
  
  // Calculate magnitude spectrum
  for (uint16_t i = 0; i < FFT_SIZE / 2; i++) {
    float real = state->fft_input[i];
    float imag = state->fft_output[i];
    state->magnitude_spectrum[i] = sqrtf(real * real + imag * imag);
  }
}

static float detect_peak_frequency(const float *spectrum, uint16_t size) {
  uint16_t peak_bin = 0;
  float peak_magnitude = 0.0f;
  
  // Find the bin with maximum magnitude (skip DC component)
  for (uint16_t i = 1; i < size - 1; i++) {
    if (spectrum[i] > peak_magnitude) {
      peak_magnitude = spectrum[i];
      peak_bin = i;
    }
  }
  
  // Convert bin to frequency
  if (peak_magnitude > 0.0f) {
    float bin_freq = (float)peak_bin * (FREQ_MAX_MHZ - FREQ_MIN_MHZ) / size;
    return FREQ_MIN_MHZ + bin_freq;
  }
  
  return 0.0f;
}

static float calculate_snr(const float *spectrum, uint16_t peak_bin, uint16_t size) {
  if (peak_bin == 0 || peak_bin >= size) {
    return MIN_SNR_DB;
  }
  
  float signal_power = spectrum[peak_bin] * spectrum[peak_bin];
  
  // Estimate noise power from surrounding bins (excluding peak and harmonics)
  float noise_sum = 0.0f;
  uint16_t noise_samples = 0;
  
  for (uint16_t i = 1; i < size; i++) {
    // Skip peak bin and potential harmonic bins
    bool is_harmonic = false;
    for (uint8_t h = 1; h <= MAX_HARMONICS; h++) {
      if (i == peak_bin * h && i < size) {
        is_harmonic = true;
        break;
      }
    }
    
    if (!is_harmonic && abs((int)i - (int)peak_bin) > 5) {
      noise_sum += spectrum[i] * spectrum[i];
      noise_samples++;
    }
  }
  
  if (noise_samples > 0) {
    float noise_power = noise_sum / noise_samples;
    if (noise_power > 0.0f) {
      return 10.0f * log10f(signal_power / noise_power);
    }
  }
  
  return MIN_SNR_DB;
}

static float raw_to_frequency_mhz(uint16_t raw_value) {
  return FREQ_MIN_MHZ + ((float)raw_value / 65535.0f) * (FREQ_MAX_MHZ - FREQ_MIN_MHZ);
}

static uint16_t frequency_to_raw(float frequency_mhz) {
  if (frequency_mhz < FREQ_MIN_MHZ) frequency_mhz = FREQ_MIN_MHZ;
  if (frequency_mhz > FREQ_MAX_MHZ) frequency_mhz = FREQ_MAX_MHZ;
  
  return (uint16_t)(((frequency_mhz - FREQ_MIN_MHZ) / (FREQ_MAX_MHZ - FREQ_MIN_MHZ)) * 65535.0f);
}

static float dbm_from_adc(uint16_t adc_value) {
  // Convert 12-bit ADC value to dBm
  // Assuming 3.3V reference and appropriate RF frontend scaling
  float voltage = (float)adc_value * 3.3f / ADC_RESOLUTION;
  
  // Convert to dBm (this would need calibration with actual RF frontend)
  // Placeholder formula - actual conversion depends on RF frontend design
  return MIN_SIGNAL_LEVEL_DBM + (voltage / 3.3f) * 60.0f; // -100 to -40 dBm range
}

static bool is_frequency_stable(FrequencyDetectorState *state) {
  // Check if frequency measurements are stable within tolerance
  if (state->history_index < FREQUENCY_STABILITY_SAMPLES) {
    return false; // Not enough samples yet
  }
  
  float mean = 0.0f;
  for (uint8_t i = 0; i < FREQUENCY_STABILITY_SAMPLES; i++) {
    mean += state->frequency_history[i];
  }
  mean /= FREQUENCY_STABILITY_SAMPLES;
  
  // Calculate standard deviation
  float variance = 0.0f;
  for (uint8_t i = 0; i < FREQUENCY_STABILITY_SAMPLES; i++) {
    float diff = state->frequency_history[i] - mean;
    variance += diff * diff;
  }
  variance /= FREQUENCY_STABILITY_SAMPLES;
  float std_dev = sqrtf(variance);
  
  // Frequency is stable if standard deviation is less than 1 MHz
  return std_dev < 1.0f;
}

// Simplified radix-2 FFT implementation
static void fft_radix2(float *real, float *imag, uint16_t size) {
  // Bit-reverse the input
  bit_reverse(real, size);
  bit_reverse(imag, size);
  
  // Perform FFT stages
  for (uint16_t stage = 1; stage <= log2f(size); stage++) {
    uint16_t m = 1 << stage;
    uint16_t m2 = m >> 1;
    
    float w_real = cosf(-2.0f * M_PI / m);
    float w_imag = sinf(-2.0f * M_PI / m);
    
    for (uint16_t i = 0; i < size; i += m) {
      float wr = 1.0f;
      float wi = 0.0f;
      
      for (uint16_t j = 0; j < m2; j++) {
        uint16_t k = i + j;
        uint16_t l = k + m2;
        
        float tr = wr * real[l] - wi * imag[l];
        float ti = wr * imag[l] + wi * real[l];
        
        real[l] = real[k] - tr;
        imag[l] = imag[k] - ti;
        real[k] = real[k] + tr;
        imag[k] = imag[k] + ti;
        
        float temp_wr = wr;
        wr = wr * w_real - wi * w_imag;
        wi = wi * w_real + temp_wr * w_imag;
      }
    }
  }
}

static void bit_reverse(float *data, uint16_t size) {
  uint16_t j = 0;
  for (uint16_t i = 0; i < size; i++) {
    if (i < j) {
      float temp = data[i];
      data[i] = data[j];
      data[j] = temp;
    }
    
    uint16_t k = size >> 1;
    while (j & k) {
      j &= ~k;
      k >>= 1;
    }
    j |= k;
  }
}