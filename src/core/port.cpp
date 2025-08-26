#include "port.h"
#include "../hardware/gpio_controller.h"
#include "../hardware/led_controller.h"
#include "../utils/logger.h"

namespace splitter::core {

Port::Port(int id, hardware::GpioController* gpio, hardware::LedController* led)
    : id_(id)
    , gpio_controller_(gpio)
    , led_controller_(led)
    , last_health_check_(std::chrono::steady_clock::now()) {
    
    state_.id = id;
    config_.name = "Port " + std::to_string(id + 1);
    
    enable_gpio_pin_ = GPIO_BASE_PIN + id;
    status_led_pin_ = STATUS_LED_BASE_PIN + id;
    signal_led_pin_ = SIGNAL_LED_BASE_PIN + id;
}

Port::~Port() {
    if (enabled_) {
        disable();
    }
}

bool Port::initialize() {
    if (!gpio_controller_ || !led_controller_) {
        set_error("Missing hardware controllers");
        return false;
    }
    
    if (!gpio_controller_->configure_output_pin(enable_gpio_pin_)) {
        set_error("Failed to configure enable GPIO pin");
        return false;
    }
    
    if (!led_controller_->configure_led(status_led_pin_)) {
        set_error("Failed to configure status LED");
        return false;
    }
    
    if (!led_controller_->configure_led(signal_led_pin_)) {
        set_error("Failed to configure signal LED");
        return false;
    }
    
    gpio_controller_->set_pin_low(enable_gpio_pin_);
    update_leds();
    
    clear_error();
    healthy_ = true;
    
    utils::Logger::debug("Port {} initialized successfully", id_);
    return true;
}

bool Port::enable() {
    if (!gpio_controller_) {
        set_error("GPIO controller not available");
        return false;
    }
    
    if (!gpio_controller_->set_pin_high(enable_gpio_pin_)) {
        set_error("Failed to enable port via GPIO");
        return false;
    }
    
    enabled_ = true;
    state_.enabled = true;
    state_.last_update = std::chrono::system_clock::now();
    
    update_leds();
    clear_error();
    
    utils::Logger::info("Port {} enabled", id_);
    return true;
}

bool Port::disable() {
    if (!gpio_controller_) {
        set_error("GPIO controller not available");
        return false;
    }
    
    if (!gpio_controller_->set_pin_low(enable_gpio_pin_)) {
        set_error("Failed to disable port via GPIO");
        return false;
    }
    
    enabled_ = false;
    signal_detected_ = false;
    state_.enabled = false;
    state_.signal_detected = false;
    state_.frequency_mhz = 0.0;
    state_.signal_level_dbm = -100.0;
    state_.last_update = std::chrono::system_clock::now();
    
    update_leds();
    clear_error();
    
    utils::Logger::info("Port {} disabled", id_);
    return true;
}

bool Port::is_enabled() const {
    return enabled_;
}

PortState Port::get_state() const {
    state_.enabled = enabled_;
    state_.signal_detected = signal_detected_;
    state_.healthy = healthy_;
    return state_;
}

PortConfig Port::get_configuration() const {
    return config_;
}

bool Port::set_configuration(const PortConfig& config) {
    if (config.min_frequency_mhz >= config.max_frequency_mhz) {
        set_error("Invalid frequency range");
        return false;
    }
    
    if (config.gain_db < -30 || config.gain_db > 30) {
        set_error("Invalid gain value");
        return false;
    }
    
    config_ = config;
    state_.last_update = std::chrono::system_clock::now();
    
    utils::Logger::info("Port {} configuration updated: {}", id_, config_.name);
    return true;
}

void Port::update_frequency(double frequency_mhz) {
    state_.frequency_mhz = frequency_mhz;
    
    if (config_.signal_detection_enabled && enabled_) {
        bool in_range = (frequency_mhz >= config_.min_frequency_mhz && 
                        frequency_mhz <= config_.max_frequency_mhz);
        signal_detected_ = (frequency_mhz > 0.0) && in_range;
        state_.signal_detected = signal_detected_;
        
        update_leds();
    }
    
    state_.last_update = std::chrono::system_clock::now();
}

void Port::update_signal_level(double level_dbm) {
    state_.signal_level_dbm = level_dbm;
    state_.last_update = std::chrono::system_clock::now();
}

void Port::check_health() {
    auto now = std::chrono::steady_clock::now();
    
    if (now - last_health_check_ < std::chrono::seconds(1)) {
        return;
    }
    
    last_health_check_ = now;
    
    bool gpio_healthy = gpio_controller_ && gpio_controller_->is_pin_healthy(enable_gpio_pin_);
    bool led_healthy = led_controller_ && 
                      led_controller_->is_led_healthy(status_led_pin_) &&
                      led_controller_->is_led_healthy(signal_led_pin_);
    
    bool was_healthy = healthy_;
    healthy_ = gpio_healthy && led_healthy;
    
    if (was_healthy && !healthy_) {
        set_error("Hardware health check failed");
        utils::Logger::warning("Port {} health check failed", id_);
    } else if (!was_healthy && healthy_) {
        clear_error();
        utils::Logger::info("Port {} health recovered", id_);
    }
    
    state_.healthy = healthy_;
}

void Port::update_leds() {
    if (!led_controller_) {
        return;
    }
    
    if (enabled_) {
        led_controller_->set_led_on(status_led_pin_);
        
        if (signal_detected_) {
            led_controller_->set_led_on(signal_led_pin_);
        } else {
            led_controller_->set_led_off(signal_led_pin_);
        }
    } else {
        led_controller_->set_led_off(status_led_pin_);
        led_controller_->set_led_off(signal_led_pin_);
    }
}

void Port::set_error(const std::string& error) {
    state_.error_message = error;
    healthy_ = false;
    utils::Logger::error("Port {} error: {}", id_, error);
}

void Port::clear_error() {
    state_.error_message.clear();
}

} // namespace splitter::core