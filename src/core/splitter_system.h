#pragma once

#include "splitter_port.h"
#include "../hardware/gpio_controller.h"
#include "../hardware/frequency_detector.h"
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <map>
#include <functional>

namespace core {

struct SystemStatus {
    std::vector<PortStatus> port_statuses;
    int enabled_ports;
    int active_ports;
    bool system_healthy;
    std::string system_error;
    std::chrono::system_clock::time_point last_update;
};

using StatusCallback = std::function<void(const SystemStatus&)>;

class SplitterSystem {
public:
    static constexpr int NUM_PORTS = 32;
    static constexpr int GPIO_BASE_ENABLE_LED = 100;  // GPIO 100-131 for enable LEDs
    static constexpr int GPIO_BASE_SIGNAL_LED = 132;  // GPIO 132-163 for signal LEDs
    
    SplitterSystem();
    ~SplitterSystem();

    bool initialize();
    void shutdown();
    
    // Port control
    bool enable_port(int port_number);
    bool disable_port(int port_number);
    bool enable_all_ports();
    bool disable_all_ports();
    
    // Status and monitoring
    SystemStatus get_system_status() const;
    PortStatus get_port_status(int port_number) const;
    std::vector<PortStatus> get_all_port_statuses() const;
    
    bool is_port_enabled(int port_number) const;
    bool is_signal_detected(int port_number) const;
    int get_enabled_port_count() const;
    int get_active_port_count() const;
    
    // Configuration
    bool set_port_configuration(int port_number, const std::map<std::string, std::string>& config);
    std::map<std::string, std::string> get_port_configuration(int port_number) const;
    
    // Callbacks for status updates
    void register_status_callback(StatusCallback callback);
    void unregister_status_callback();
    
    // Maintenance
    bool run_self_test();
    bool calibrate_frequency_detectors();

private:
    std::vector<std::unique_ptr<SplitterPort>> ports_;
    std::shared_ptr<hardware::GPIOController> gpio_controller_;
    std::shared_ptr<hardware::FrequencyDetectorArray> freq_detector_array_;
    
    mutable std::mutex system_mutex_;
    std::atomic<bool> initialized_;
    std::atomic<bool> monitoring_;
    
    std::unique_ptr<std::thread> monitor_thread_;
    StatusCallback status_callback_;
    
    void monitoring_loop();
    void update_all_displays();
    bool validate_port_number(int port_number) const;
    void notify_status_change();
};

} // namespace core