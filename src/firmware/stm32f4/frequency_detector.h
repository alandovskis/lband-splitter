#ifndef FREQUENCY_DETECTOR_H
#define FREQUENCY_DETECTOR_H

#include <stdbool.h>
#include <stdint.h>

// L-band frequency range constants
#define FREQ_MIN_MHZ 950.0f
#define FREQ_MAX_MHZ 2150.0f
#define FREQ_RESOLUTION_HZ 1000.0f // 1 kHz resolution

// ADC and timing constants
#define ADC_RESOLUTION 4096 // 12-bit ADC
#define SAMPLE_BUFFER_SIZE 256
#define FFT_SIZE 256
#define MEASUREMENT_TIMEOUT_MS 1000

// Signal quality thresholds
#define MIN_SNR_DB -30.0f
#define MAX_SNR_DB 40.0f
#define MIN_SIGNAL_LEVEL_DBM -100.0f
#define NOISE_FLOOR_DBM -95.0f

// Frequency detection algorithm parameters
#define PEAK_DETECTION_THRESHOLD 3.0f // dB above noise floor
#define FREQUENCY_STABILITY_SAMPLES 10
#define MAX_HARMONICS 5

typedef struct {
  float frequency_mhz;
  float snr_db;
  float signal_level_dbm;
  uint16_t raw_frequency;
  uint16_t raw_snr;
  uint32_t timestamp;
  bool valid;
  uint8_t port_id;
} FrequencyReading;

typedef struct {
  // ADC sample buffer
  uint16_t sample_buffer[SAMPLE_BUFFER_SIZE];
  uint16_t sample_index;
  bool buffer_full;

  // FFT processing
  float fft_input[FFT_SIZE];
  float fft_output[FFT_SIZE];
  float magnitude_spectrum[FFT_SIZE / 2];

  // Frequency detection state
  float current_frequency_mhz;
  float frequency_history[FREQUENCY_STABILITY_SAMPLES];
  uint8_t history_index;

  // Signal quality metrics
  float noise_floor_dbm;
  float signal_power_dbm;
  float snr_db;

  // Calibration data
  float frequency_calibration_offset;
  float amplitude_calibration_factor;
  bool calibrated;

  // Port configuration
  uint8_t port_id;
  bool enabled;
  uint32_t measurement_count;
  uint32_t last_measurement_time;
} FrequencyDetectorState;

// Public API
void frequency_detector_init(FrequencyDetectorState *state);
bool frequency_detector_measure(FrequencyDetectorState *state,
                                FrequencyReading *reading);
void frequency_detector_adc_callback(FrequencyDetectorState *state,
                                     uint16_t adc_value);
bool frequency_detector_calibrate(FrequencyDetectorState *state);
void frequency_detector_reset(FrequencyDetectorState *state);

// Internal functions
static void process_fft(FrequencyDetectorState *state);
static float detect_peak_frequency(const float *spectrum, uint16_t size);
static float calculate_snr(const float *spectrum, uint16_t peak_bin,
                           uint16_t size);
static float raw_to_frequency_mhz(uint16_t raw_value);
static uint16_t frequency_to_raw(float frequency_mhz);
static float dbm_from_adc(uint16_t adc_value);
static bool is_frequency_stable(FrequencyDetectorState *state);

#endif // FREQUENCY_DETECTOR_H