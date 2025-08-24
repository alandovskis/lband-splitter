#include "splitter_port.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace core {

SplitterPort::SplitterPort(int port_number, 
                           int enable_led_pin, 
                           int signal_led_pin,
                           std::shared_ptr<hardware::GPIOController> gpio_controller,
                           std::shared_ptr<hardware::FrequencyDetector> freq_detector)
    : port_number_(port_number)
    , enable_led_pin_(enable_led_pin)
    , signal_led_pin_(signal_led_pin)
    , gpio_controller_(gpio_controller)
    , freq_detector_(freq_detector)
    , state_(PortState::DISABLED) {
}

SplitterPort::~SplitterPort() {
    disable();
}

bool SplitterPort::initialize() {
    if (!gpio_controller_ || !freq_detector_) {
        set_error("Missing hardware controllers");
        return false;
    }
    
    // Initialize GPIO pins for LEDs
    auto enable_led = gpio_controller_->get_pin(enable_led_pin_, hardware::GPIODirection::OUTPUT);
    auto signal_led = gpio_controller_->get_pin(signal_led_pin_, hardware::GPIODirection::OUTPUT);
    
    if (!enable_led || !signal_led) {
        set_error("Failed to initialize GPIO pins");
        return false;
    }
    
    // Initialize frequency detector
    if (!freq_detector_->initialize()) {
        set_error("Failed to initialize frequency detector");
        return false;
    }
    
    // Set initial LED states (both off)
    enable_led->set_value(hardware::GPIOValue::LOW);
    signal_led->set_value(hardware::GPIOValue::LOW);
    
    clear_error();
    return true;
}

bool SplitterPort::enable() {
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    if (state_ == PortState::ERROR) {
        return false;
    }
    
    state_ = PortState::ENABLED;
    
    // Start frequency monitoring
    freq_detector_->start_monitoring();
    
    // Update LEDs
    if (!update_leds()) {
        state_ = PortState::ERROR;
        set_error("Failed to update LED status");
        return false;
    }
    
    std::cout << "Port " << port_number_ << " enabled" << std::endl;
    return true;
}

bool SplitterPort::disable() {
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    state_ = PortState::DISABLED;
    
    // Stop frequency monitoring
    freq_detector_->stop_monitoring();
    
    // Update LEDs
    update_leds();
    
    std::cout << "Port " << port_number_ << " disabled" << std::endl;
    return true;
}

PortStatus SplitterPort::get_status() const {
    std::lock_guard<std::mutex> lock(status_mutex_);
    
    PortStatus status;
    status.port_number = port_number_;
    status.state = state_.load();
    status.error_message = error_message_;
    status.last_update = std::chrono::system_clock::now();
    
    if (state_ == PortState::ENABLED) {
        auto measurement = freq_detector_->get_last_measurement();
        status.signal_detected = measurement.signal_detected;
        status.center_frequency_mhz = measurement.center_frequency_mhz;
        status.power_level_dbm = measurement.power_level_dbm;
    } else {
        status.signal_detected = false;
        status.center_frequency_mhz = 0.0;
        status.power_level_dbm = -100.0;
    }
    
    return status;
}

bool SplitterPort::is_enabled() const {
    return state_.load() == PortState::ENABLED;
}

bool SplitterPort::has_signal() const {
    if (state_.load() != PortState::ENABLED) {
        return false;
    }
    return freq_detector_->is_signal_detected();
}

double SplitterPort::get_frequency() const {
    if (state_.load() != PortState::ENABLED) {
        return 0.0;
    }
    return freq_detector_->get_center_frequency();
}

double SplitterPort::get_power_level() const {
    if (state_.load() != PortState::ENABLED) {
        return -100.0;
    }
    return freq_detector_->get_power_level();
}

void SplitterPort::update_display() {
    if (state_.load() == PortState::ENABLED) {
        double freq = get_frequency();
        if (freq > 0) {
            format_frequency_display(freq);
        }
    }
    update_leds();
}

void SplitterPort::set_error(const std::string& error_msg) {
    error_message_ = error_msg;
    state_ = PortState::ERROR;
    std::cerr << "Port " << port_number_ << " error: " << error_msg << std::endl;
}

void SplitterPort::clear_error() {
    error_message_.clear();
    if (state_ == PortState::ERROR) {
        state_ = PortState::DISABLED;
    }
}

bool SplitterPort::update_leds() {
    // Enable LED: ON when port is enabled, OFF when disabled or error
    hardware::GPIOValue enable_led_value = hardware::GPIOValue::LOW;
    if (state_ == PortState::ENABLED) {
        enable_led_value = hardware::GPIOValue::HIGH;
    }
    
    // Signal LED: ON when signal is detected, OFF otherwise
    hardware::GPIOValue signal_led_value = hardware::GPIOValue::LOW;
    if (state_ == PortState::ENABLED && has_signal()) {
        signal_led_value = hardware::GPIOValue::HIGH;
    }
    
    bool success = true;
    success &= gpio_controller_->set_pin_value(enable_led_pin_, enable_led_value);
    success &= gpio_controller_->set_pin_value(signal_led_pin_, signal_led_value);
    
    return success;
}

void SplitterPort::format_frequency_display(double frequency_mhz) {
    // In a real implementation, this would send the frequency value
    // to a small display connected via I2C or SPI
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << frequency_mhz << " MHz";
    
    // Simulate display update
    std::cout << "Port " << port_number_ << " display: " << oss.str() << std::endl;
}

} // namespace core