#pragma once

#include <cstdint>
#include <chrono>

extern "C" {
#include "stm32f4xx_hal.h"
}

namespace stm32f4 {

struct FrequencyReading {
    double frequency_mhz{0.0};
    double snr_db{0.0};
    double signal_level_dbm{-100.0};
    uint16_t raw_frequency{0};
    uint16_t raw_snr{0};
    std::chrono::steady_clock::time_point timestamp;
    bool valid{false};
    uint8_t port_id{0};
};

class FrequencyDetector {
public:
    FrequencyDetector();
    ~FrequencyDetector();

    bool initialize();
    void cleanup();
    
    bool start_measurement();
    bool stop_measurement();
    
    FrequencyReading get_last_reading() const;
    bool read_frequency_and_snr(FrequencyReading& reading);
    
    bool calibrate();
    void reset();
    
    bool is_enabled() const { return enabled_; }
    bool is_calibrated() const { return calibrated_; }
    uint32_t get_measurement_count() const { return measurement_count_; }

private:
    void process_adc_data();
    double calculate_frequency_from_raw(uint16_t raw_value) const;
    double calculate_snr_from_raw(uint16_t raw_value) const;
    double calculate_signal_level_from_raw(uint16_t raw_value) const;
    
    static constexpr double FREQUENCY_SCALE_FACTOR = 0.5; // MHz per LSB
    static constexpr double FREQUENCY_OFFSET = 950.0;     // MHz baseline
    static constexpr double SNR_SCALE_FACTOR = 0.1;      // dB per LSB
    static constexpr double SIGNAL_LEVEL_OFFSET = -100.0; // dBm baseline
    
    bool enabled_{false};
    bool calibrated_{false};
    uint32_t measurement_count_{0};
    FrequencyReading last_reading_;
    
    // STM32 HAL handles
    ADC_HandleTypeDef hadc1_;
    TIM_HandleTypeDef htim2_;
    DMA_HandleTypeDef hdma_adc1_;
    
    // ADC buffer for DMA
    static constexpr size_t ADC_BUFFER_SIZE = 1024;
    uint16_t adc_buffer_[ADC_BUFFER_SIZE];
    volatile bool adc_conversion_complete_{false};
};

} // namespace stm32f4