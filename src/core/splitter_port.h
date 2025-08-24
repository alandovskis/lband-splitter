#pragma once

#include "../hardware/gpio_controller.h"
#include "../hardware/frequency_detector.h"
#include <memory>
#include <string>
#include <mutex>
#include <atomic>

namespace core {

enum class PortState {
    DISABLED,
    ENABLED,
    ERROR
};

struct PortStatus {
    int port_number;
    PortState state;
    bool signal_detected;
    double center_frequency_mhz;
    double power_level_dbm;
    std::string error_message;
    std::chrono::system_clock::time_point last_update;
};

class SplitterPort {
public:
    SplitterPort(int port_number, 
                 int enable_led_pin, 
                 int signal_led_pin,
                 std::shared_ptr<hardware::GPIOController> gpio_controller,
                 std::shared_ptr<hardware::FrequencyDetector> freq_detector);
    
    ~SplitterPort();

    bool initialize();
    bool enable();
    bool disable();
    
    PortStatus get_status() const;
    bool is_enabled() const;
    bool has_signal() const;
    double get_frequency() const;
    double get_power_level() const;
    
    void update_display();
    void set_error(const std::string& error_msg);
    void clear_error();

private:
    int port_number_;
    int enable_led_pin_;
    int signal_led_pin_;
    
    std::shared_ptr<hardware::GPIOController> gpio_controller_;
    std::shared_ptr<hardware::FrequencyDetector> freq_detector_;
    
    std::atomic<PortState> state_;
    std::string error_message_;
    mutable std::mutex status_mutex_;
    
    bool update_leds();
    void format_frequency_display(double frequency_mhz);
};

} // namespace core