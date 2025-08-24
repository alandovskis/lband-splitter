#include "frequency_detector.h"
#include <iostream>
#include <fstream>
#include <random>
#include <algorithm>

namespace hardware {

FrequencyDetector::FrequencyDetector(int port_number)
    : port_number_(port_number), monitoring_(false), initialized_(false) {
    last_measurement_.center_frequency_mhz = 0.0;
    last_measurement_.power_level_dbm = -100.0;
    last_measurement_.signal_detected = false;
    last_measurement_.timestamp = std::chrono::system_clock::now();
}

FrequencyDetector::~FrequencyDetector() {
    stop_monitoring();
}

bool FrequencyDetector::initialize() {
    if (initialized_.load()) {
        return true;
    }
    
    if (!configure_adc()) {
        std::cerr << "Failed to configure ADC for port " << port_number_ << std::endl;
        return false;
    }
    
    initialized_.store(true);
    return true;
}

void FrequencyDetector::start_monitoring() {
    if (!initialized_.load() || monitoring_.load()) {
        return;
    }
    
    monitoring_.store(true);
    monitor_thread_ = std::make_unique<std::thread>(&FrequencyDetector::monitor_loop, this);
}

void FrequencyDetector::stop_monitoring() {
    monitoring_.store(false);
    if (monitor_thread_ && monitor_thread_->joinable()) {
        monitor_thread_->join();
    }
}

FrequencyMeasurement FrequencyDetector::get_last_measurement() const {
    std::lock_guard<std::mutex> lock(measurement_mutex_);
    return last_measurement_;
}

bool FrequencyDetector::is_signal_detected() const {
    std::lock_guard<std::mutex> lock(measurement_mutex_);
    return last_measurement_.signal_detected;
}

double FrequencyDetector::get_center_frequency() const {
    std::lock_guard<std::mutex> lock(measurement_mutex_);
    return last_measurement_.center_frequency_mhz;
}

double FrequencyDetector::get_power_level() const {
    std::lock_guard<std::mutex> lock(measurement_mutex_);
    return last_measurement_.power_level_dbm;
}

void FrequencyDetector::monitor_loop() {
    while (monitoring_.load()) {
        double frequency, power;
        if (read_adc_values(frequency, power)) {
            std::lock_guard<std::mutex> lock(measurement_mutex_);
            
            last_measurement_.center_frequency_mhz = frequency;
            last_measurement_.power_level_dbm = power;
            last_measurement_.signal_detected = power > -80.0; // -80 dBm threshold
            last_measurement_.timestamp = std::chrono::system_clock::now();
        }
        
        // Update every 100ms
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool FrequencyDetector::read_adc_values(double& frequency, double& power) {
    // In a real implementation, this would read from actual ADC hardware
    // For now, simulate L-band frequencies (1-2 GHz) with some noise
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> freq_dist(1000.0, 2000.0); // 1-2 GHz
    static std::uniform_real_distribution<> power_dist(-90.0, -30.0);   // -90 to -30 dBm
    static std::uniform_real_distribution<> noise(0.9, 1.1);            // ±10% noise
    
    // Simulate signal presence based on port number (some ports have signals)
    bool has_signal = (port_number_ % 4 == 0); // Every 4th port has a signal
    
    if (has_signal) {
        frequency = freq_dist(gen) * noise(gen);
        power = power_dist(gen) * noise(gen);
    } else {
        frequency = 0.0;
        power = -100.0; // No signal
    }
    
    return true;
}

bool FrequencyDetector::configure_adc() {
    // In a real implementation, this would configure the ADC hardware
    // via I2C, SPI, or other interfaces
    
    std::cout << "Configuring ADC for port " << port_number_ << std::endl;
    return true;
}

FrequencyDetectorArray::FrequencyDetectorArray(int num_ports) : num_ports_(num_ports) {
    detectors_.reserve(num_ports_);
    for (int i = 0; i < num_ports_; ++i) {
        detectors_.push_back(std::make_unique<FrequencyDetector>(i));
    }
}

FrequencyDetectorArray::~FrequencyDetectorArray() {
    stop_all_monitoring();
}

bool FrequencyDetectorArray::initialize() {
    std::lock_guard<std::mutex> lock(array_mutex_);
    
    bool success = true;
    for (auto& detector : detectors_) {
        if (!detector->initialize()) {
            success = false;
        }
    }
    
    return success;
}

void FrequencyDetectorArray::start_all_monitoring() {
    std::lock_guard<std::mutex> lock(array_mutex_);
    
    for (auto& detector : detectors_) {
        detector->start_monitoring();
    }
}

void FrequencyDetectorArray::stop_all_monitoring() {
    std::lock_guard<std::mutex> lock(array_mutex_);
    
    for (auto& detector : detectors_) {
        detector->stop_monitoring();
    }
}

FrequencyMeasurement FrequencyDetectorArray::get_measurement(int port) const {
    std::lock_guard<std::mutex> lock(array_mutex_);
    
    if (port >= 0 && port < num_ports_) {
        return detectors_[port]->get_last_measurement();
    }
    
    return FrequencyMeasurement{};
}

std::vector<FrequencyMeasurement> FrequencyDetectorArray::get_all_measurements() const {
    std::lock_guard<std::mutex> lock(array_mutex_);
    
    std::vector<FrequencyMeasurement> measurements;
    measurements.reserve(num_ports_);
    
    for (const auto& detector : detectors_) {
        measurements.push_back(detector->get_last_measurement());
    }
    
    return measurements;
}

bool FrequencyDetectorArray::is_signal_detected(int port) const {
    std::lock_guard<std::mutex> lock(array_mutex_);
    
    if (port >= 0 && port < num_ports_) {
        return detectors_[port]->is_signal_detected();
    }
    
    return false;
}

int FrequencyDetectorArray::get_active_port_count() const {
    std::lock_guard<std::mutex> lock(array_mutex_);
    
    int count = 0;
    for (const auto& detector : detectors_) {
        if (detector->is_signal_detected()) {
            count++;
        }
    }
    
    return count;
}

} // namespace hardware