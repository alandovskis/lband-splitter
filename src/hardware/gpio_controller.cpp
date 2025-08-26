#include "gpio_controller.h"
#include "../utils/logger.h"

#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

namespace splitter::hardware {

GpioController::GpioController() = default;

GpioController::~GpioController() {
    cleanup();
}

bool GpioController::initialize() {
    if (initialized_) {
        return true;
    }
    
    if (!std::filesystem::exists(GPIO_BASE_PATH)) {
        last_error_ = "GPIO sysfs not available";
        utils::Logger::error("GPIO sysfs not available at {}", GPIO_BASE_PATH);
        return false;
    }
    
    initialized_ = true;
    utils::Logger::info("GPIO controller initialized");
    return true;
}

void GpioController::cleanup() {
    if (!initialized_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(pins_mutex_);
    
    for (const auto& [pin, info] : pins_) {
        if (info.configured) {
            unexport_pin(pin);
        }
    }
    
    pins_.clear();
    initialized_ = false;
    
    utils::Logger::info("GPIO controller cleanup complete");
}

bool GpioController::configure_input_pin(int pin) {
    std::lock_guard<std::mutex> lock(pins_mutex_);
    
    if (!export_pin(pin)) {
        return false;
    }
    
    if (!set_pin_direction(pin, PinDirection::INPUT)) {
        unexport_pin(pin);
        return false;
    }
    
    pins_[pin] = {PinDirection::INPUT, PinState::LOW, true, true, ""};
    utils::Logger::debug("Configured GPIO pin {} as input", pin);
    return true;
}

bool GpioController::configure_output_pin(int pin) {
    std::lock_guard<std::mutex> lock(pins_mutex_);
    
    if (!export_pin(pin)) {
        return false;
    }
    
    if (!set_pin_direction(pin, PinDirection::OUTPUT)) {
        unexport_pin(pin);
        return false;
    }
    
    if (!write_pin_value(pin, PinState::LOW)) {
        unexport_pin(pin);
        return false;
    }
    
    pins_[pin] = {PinDirection::OUTPUT, PinState::LOW, true, true, ""};
    utils::Logger::debug("Configured GPIO pin {} as output", pin);
    return true;
}

bool GpioController::set_pin_high(int pin) {
    return set_pin_state(pin, PinState::HIGH);
}

bool GpioController::set_pin_low(int pin) {
    return set_pin_state(pin, PinState::LOW);
}

bool GpioController::set_pin_state(int pin, PinState state) {
    std::lock_guard<std::mutex> lock(pins_mutex_);
    
    auto it = pins_.find(pin);
    if (it == pins_.end() || !it->second.configured) {
        last_error_ = "Pin " + std::to_string(pin) + " not configured";
        return false;
    }
    
    if (it->second.direction != PinDirection::OUTPUT) {
        last_error_ = "Pin " + std::to_string(pin) + " not configured as output";
        return false;
    }
    
    if (!write_pin_value(pin, state)) {
        it->second.healthy = false;
        it->second.error_message = last_error_;
        return false;
    }
    
    it->second.last_state = state;
    it->second.healthy = true;
    it->second.error_message.clear();
    
    return true;
}

PinState GpioController::get_pin_state(int pin) const {
    std::lock_guard<std::mutex> lock(pins_mutex_);
    
    auto it = pins_.find(pin);
    if (it == pins_.end() || !it->second.configured) {
        return PinState::LOW;
    }
    
    if (it->second.direction == PinDirection::INPUT) {
        return read_pin_value(pin);
    } else {
        return it->second.last_state;
    }
}

bool GpioController::is_pin_configured(int pin) const {
    std::lock_guard<std::mutex> lock(pins_mutex_);
    
    auto it = pins_.find(pin);
    return it != pins_.end() && it->second.configured;
}

bool GpioController::is_pin_healthy(int pin) const {
    std::lock_guard<std::mutex> lock(pins_mutex_);
    
    auto it = pins_.find(pin);
    return it != pins_.end() && it->second.healthy;
}

bool GpioController::is_healthy() const {
    if (!initialized_) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(pins_mutex_);
    
    for (const auto& [pin, info] : pins_) {
        if (!info.healthy) {
            return false;
        }
    }
    
    return true;
}

std::string GpioController::get_last_error() const {
    return last_error_;
}

bool GpioController::export_pin(int pin) {
    if (std::filesystem::exists(get_gpio_path(pin, ""))) {
        return true;
    }
    
    if (!write_to_sysfs(GPIO_EXPORT_PATH, std::to_string(pin))) {
        last_error_ = "Failed to export GPIO pin " + std::to_string(pin);
        return false;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    int retries = 10;
    while (!std::filesystem::exists(get_gpio_path(pin, "")) && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    if (!std::filesystem::exists(get_gpio_path(pin, ""))) {
        last_error_ = "GPIO pin " + std::to_string(pin) + " not available after export";
        return false;
    }
    
    return true;
}

bool GpioController::unexport_pin(int pin) {
    if (!std::filesystem::exists(get_gpio_path(pin, ""))) {
        return true;
    }
    
    return write_to_sysfs(GPIO_UNEXPORT_PATH, std::to_string(pin));
}

bool GpioController::set_pin_direction(int pin, PinDirection direction) {
    std::string dir_value = (direction == PinDirection::INPUT) ? "in" : "out";
    
    if (!write_to_sysfs(get_gpio_path(pin, "direction"), dir_value)) {
        last_error_ = "Failed to set direction for GPIO pin " + std::to_string(pin);
        return false;
    }
    
    return true;
}

bool GpioController::write_pin_value(int pin, PinState state) {
    std::string value = (state == PinState::HIGH) ? "1" : "0";
    
    if (!write_to_sysfs(get_gpio_path(pin, "value"), value)) {
        last_error_ = "Failed to write value to GPIO pin " + std::to_string(pin);
        return false;
    }
    
    return true;
}

PinState GpioController::read_pin_value(int pin) const {
    std::string value = read_from_sysfs(get_gpio_path(pin, "value"));
    return (value == "1") ? PinState::HIGH : PinState::LOW;
}

bool GpioController::write_to_sysfs(const std::string& path, const std::string& value) {
    try {
        std::ofstream file(path);
        if (!file) {
            last_error_ = "Failed to open " + path + " for writing";
            return false;
        }
        
        file << value;
        file.flush();
        
        if (file.fail()) {
            last_error_ = "Failed to write to " + path;
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        last_error_ = "Exception writing to " + path + ": " + e.what();
        return false;
    }
}

std::string GpioController::read_from_sysfs(const std::string& path) const {
    try {
        std::ifstream file(path);
        if (!file) {
            return "";
        }
        
        std::string value;
        std::getline(file, value);
        
        if (!value.empty() && value.back() == '\n') {
            value.pop_back();
        }
        
        return value;
        
    } catch (const std::exception& e) {
        utils::Logger::warning("Exception reading from {}: {}", path, e.what());
        return "";
    }
}

std::string GpioController::get_gpio_path(int pin, const std::string& file) const {
    std::string base_path = std::string(GPIO_BASE_PATH) + "/gpio" + std::to_string(pin);
    if (file.empty()) {
        return base_path;
    }
    return base_path + "/" + file;
}

} // namespace splitter::hardware