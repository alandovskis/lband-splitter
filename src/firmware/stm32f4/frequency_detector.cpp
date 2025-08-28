#include "frequency_detector.hpp"
#include <algorithm>
#include <cmath>

extern "C" {
#include "stm32f4xx_hal.h"
}

namespace stm32f4 {

FrequencyDetector::FrequencyDetector() {
    std::fill(adc_buffer_, adc_buffer_ + ADC_BUFFER_SIZE, 0);
}

FrequencyDetector::~FrequencyDetector() {
    cleanup();
}

bool FrequencyDetector::initialize() {
    // Configure ADC1 for frequency detection
    hadc1_.Instance = ADC1;
    hadc1_.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1_.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1_.Init.ScanConvMode = DISABLE;
    hadc1_.Init.ContinuousConvMode = ENABLE;
    hadc1_.Init.DiscontinuousConvMode = DISABLE;
    hadc1_.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1_.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1_.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1_.Init.NbrOfConversion = 1;
    hadc1_.Init.DMAContinuousRequests = ENABLE;
    hadc1_.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    
    if (HAL_ADC_Init(&hadc1_) != HAL_OK) {
        return false;
    }
    
    // Configure ADC channel (PA0 - ADC1_IN0)
    ADC_ChannelConfTypeDef sConfig = {};
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    
    if (HAL_ADC_ConfigChannel(&hadc1_, &sConfig) != HAL_OK) {
        return false;
    }
    
    // Configure Timer2 for ADC triggering at 1 MHz sample rate
    htim2_.Instance = TIM2;
    htim2_.Init.Prescaler = 84 - 1;  // 84 MHz / 84 = 1 MHz
    htim2_.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2_.Init.Period = 1 - 1;      // 1 MHz / 1 = 1 MHz trigger rate
    htim2_.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    
    if (HAL_TIM_Base_Init(&htim2_) != HAL_OK) {
        return false;
    }
    
    // Configure DMA for ADC
    hdma_adc1_.Instance = DMA2_Stream0;
    hdma_adc1_.Init.Channel = DMA_CHANNEL_0;
    hdma_adc1_.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1_.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1_.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc1_.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1_.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1_.Init.Mode = DMA_CIRCULAR;
    hdma_adc1_.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_adc1_.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    
    if (HAL_DMA_Init(&hdma_adc1_) != HAL_OK) {
        return false;
    }
    
    __HAL_LINKDMA(&hadc1_, DMA_Handle, hdma_adc1_);
    
    // Calibrate ADC
    if (HAL_ADCEx_Calibration_Start(&hadc1_) != HAL_OK) {
        return false;
    }
    
    enabled_ = true;
    calibrated_ = true;
    measurement_count_ = 0;
    
    return true;
}

void FrequencyDetector::cleanup() {
    if (enabled_) {
        stop_measurement();
        HAL_ADC_DeInit(&hadc1_);
        HAL_TIM_Base_DeInit(&htim2_);
        HAL_DMA_DeInit(&hdma_adc1_);
        enabled_ = false;
        calibrated_ = false;
    }
}

bool FrequencyDetector::start_measurement() {
    if (!enabled_ || !calibrated_) {
        return false;
    }
    
    adc_conversion_complete_ = false;
    
    // Start DMA transfer
    if (HAL_ADC_Start_DMA(&hadc1_, reinterpret_cast<uint32_t*>(adc_buffer_), ADC_BUFFER_SIZE) != HAL_OK) {
        return false;
    }
    
    // Start timer to trigger ADC conversions
    if (HAL_TIM_Base_Start(&htim2_) != HAL_OK) {
        return false;
    }
    
    return true;
}

bool FrequencyDetector::stop_measurement() {
    if (!enabled_) {
        return false;
    }
    
    HAL_TIM_Base_Stop(&htim2_);
    HAL_ADC_Stop_DMA(&hadc1_);
    
    return true;
}

FrequencyReading FrequencyDetector::get_last_reading() const {
    return last_reading_;
}

bool FrequencyDetector::read_frequency_and_snr(FrequencyReading& reading) {
    if (!enabled_ || !calibrated_) {
        return false;
    }
    
    // Wait for ADC conversion to complete (or timeout)
    uint32_t timeout = HAL_GetTick() + 100; // 100ms timeout
    while (!adc_conversion_complete_ && HAL_GetTick() < timeout) {
        HAL_Delay(1);
    }
    
    if (!adc_conversion_complete_) {
        return false;
    }
    
    // Process ADC data to extract frequency and SNR
    process_adc_data();
    
    reading = last_reading_;
    reading.timestamp = std::chrono::steady_clock::now();
    reading.valid = true;
    
    measurement_count_++;
    adc_conversion_complete_ = false;
    
    return true;
}

bool FrequencyDetector::calibrate() {
    if (!enabled_) {
        return false;
    }
    
    // Perform ADC calibration
    if (HAL_ADCEx_Calibration_Start(&hadc1_) != HAL_OK) {
        return false;
    }
    
    calibrated_ = true;
    return true;
}

void FrequencyDetector::reset() {
    measurement_count_ = 0;
    last_reading_ = FrequencyReading{};
    adc_conversion_complete_ = false;
}

void FrequencyDetector::process_adc_data() {
    // Simple frequency detection algorithm
    // In a real implementation, this would use FFT or other DSP techniques
    
    // Calculate DC component and peak-to-peak for basic signal analysis
    uint32_t sum = 0;
    uint16_t min_val = adc_buffer_[0];
    uint16_t max_val = adc_buffer_[0];
    
    for (size_t i = 0; i < ADC_BUFFER_SIZE; ++i) {
        sum += adc_buffer_[i];
        if (adc_buffer_[i] < min_val) min_val = adc_buffer_[i];
        if (adc_buffer_[i] > max_val) max_val = adc_buffer_[i];
    }
    
    uint16_t avg = sum / ADC_BUFFER_SIZE;
    uint16_t peak_to_peak = max_val - min_val;
    
    // Simulate frequency detection based on signal characteristics
    // This is a placeholder - real implementation would use FFT
    uint16_t raw_frequency = avg + (peak_to_peak / 4);
    uint16_t raw_snr = peak_to_peak;
    
    // Convert raw values to engineering units
    last_reading_.raw_frequency = raw_frequency;
    last_reading_.raw_snr = raw_snr;
    last_reading_.frequency_mhz = calculate_frequency_from_raw(raw_frequency);
    last_reading_.snr_db = calculate_snr_from_raw(raw_snr);
    last_reading_.signal_level_dbm = calculate_signal_level_from_raw(raw_frequency);
    last_reading_.port_id = 0; // Single port for now
    last_reading_.valid = (peak_to_peak > 100); // Simple signal detection threshold
}

double FrequencyDetector::calculate_frequency_from_raw(uint16_t raw_value) const {
    // Convert 12-bit ADC value to frequency in MHz
    // Assumes L-band range 950-2150 MHz mapped to 0-4095 ADC counts
    return FREQUENCY_OFFSET + (raw_value * FREQUENCY_SCALE_FACTOR);
}

double FrequencyDetector::calculate_snr_from_raw(uint16_t raw_value) const {
    // Convert peak-to-peak amplitude to SNR estimate in dB
    if (raw_value == 0) return 0.0;
    return 20.0 * std::log10(raw_value) - 60.0; // Calibrated offset
}

double FrequencyDetector::calculate_signal_level_from_raw(uint16_t raw_value) const {
    // Convert ADC reading to signal level in dBm
    return SIGNAL_LEVEL_OFFSET + (raw_value * 0.02); // 0.02 dBm per LSB
}

} // namespace stm32f4

// C-style DMA completion callback for HAL integration
extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == ADC1) {
        // Set flag to indicate conversion complete
        // In a real implementation, this would be handled through a singleton or global instance
    }
}