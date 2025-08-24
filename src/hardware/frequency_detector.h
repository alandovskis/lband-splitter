#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <chrono>
#include <mutex>

namespace hardware {

struct FrequencyMeasurement {
    double center_frequency_mhz;
    double power_level_dbm;
    bool signal_detected;
    std::chrono::system_clock::time_point timestamp;
};

class FrequencyDetector {
public:
    FrequencyDetector(int port_number);
    ~FrequencyDetector();

    bool initialize();
    void start_monitoring();
    void stop_monitoring();
    
    FrequencyMeasurement get_last_measurement() const;
    bool is_signal_detected() const;
    double get_center_frequency() const;
    double get_power_level() const;

private:
    int port_number_;
    std::atomic<bool> monitoring_;
    std::atomic<bool> initialized_;
    
    mutable std::mutex measurement_mutex_;
    FrequencyMeasurement last_measurement_;
    
    std::unique_ptr<std::thread> monitor_thread_;
    
    void monitor_loop();
    bool read_adc_values(double& frequency, double& power);
    bool configure_adc();
};

class FrequencyDetectorArray {
public:
    FrequencyDetectorArray(int num_ports = 32);
    ~FrequencyDetectorArray();

    bool initialize();
    void start_all_monitoring();
    void stop_all_monitoring();
    
    FrequencyMeasurement get_measurement(int port) const;
    std::vector<FrequencyMeasurement> get_all_measurements() const;
    
    bool is_signal_detected(int port) const;
    int get_active_port_count() const;

private:
    int num_ports_;
    std::vector<std::unique_ptr<FrequencyDetector>> detectors_;
    mutable std::mutex array_mutex_;
};

} // namespace hardware