#include "gpio_controller.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <algorithm>

namespace hardware {

GPIOPin::GPIOPin(int pin_number, GPIODirection direction)
    : pin_number_(pin_number), direction_(direction), initialized_(false) {
}

GPIOPin::~GPIOPin() {
    if (initialized_) {
        unexport_pin();
    }
}

bool GPIOPin::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        return true;
    }
    
    if (!export_pin()) {
        return false;
    }
    
    // Wait for the pin to be available
    usleep(100000); // 100ms
    
    if (!set_direction()) {
        unexport_pin();
        return false;
    }
    
    initialized_ = true;
    return true;
}

bool GPIOPin::export_pin() {
    std::ofstream export_file("/sys/class/gpio/export");
    if (!export_file.is_open()) {
        std::cerr << "Failed to open GPIO export file" << std::endl;
        return false;
    }
    
    export_file << pin_number_;
    export_file.close();
    
    return true;
}

bool GPIOPin::unexport_pin() {
    std::ofstream unexport_file("/sys/class/gpio/unexport");
    if (!unexport_file.is_open()) {
        return false;
    }
    
    unexport_file << pin_number_;
    unexport_file.close();
    
    return true;
}

bool GPIOPin::set_direction() {
    std::stringstream ss;
    ss << "/sys/class/gpio/gpio" << pin_number_ << "/direction";
    
    std::ofstream direction_file(ss.str());
    if (!direction_file.is_open()) {
        std::cerr << "Failed to open GPIO direction file: " << ss.str() << std::endl;
        return false;
    }
    
    if (direction_ == GPIODirection::OUTPUT) {
        direction_file << "out";
    } else {
        direction_file << "in";
    }
    
    direction_file.close();
    return true;
}

bool GPIOPin::set_value(GPIOValue value) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_ || direction_ != GPIODirection::OUTPUT) {
        return false;
    }
    
    std::stringstream ss;
    ss << "/sys/class/gpio/gpio" << pin_number_ << "/value";
    
    std::ofstream value_file(ss.str());
    if (!value_file.is_open()) {
        return false;
    }
    
    value_file << static_cast<int>(value);
    value_file.close();
    
    return true;
}

GPIOValue GPIOPin::get_value() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return GPIOValue::LOW;
    }
    
    std::stringstream ss;
    ss << "/sys/class/gpio/gpio" << pin_number_ << "/value";
    
    std::ifstream value_file(ss.str());
    if (!value_file.is_open()) {
        return GPIOValue::LOW;
    }
    
    int value;
    value_file >> value;
    value_file.close();
    
    return (value == 1) ? GPIOValue::HIGH : GPIOValue::LOW;
}

GPIOController::GPIOController() : initialized_(false) {
}

GPIOController::~GPIOController() {
    pins_.clear();
}

bool GPIOController::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    initialized_ = true;
    return true;
}

std::shared_ptr<GPIOPin> GPIOController::get_pin(int pin_number, GPIODirection direction) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto existing_pin = find_pin(pin_number);
    if (existing_pin) {
        return existing_pin;
    }
    
    auto pin = std::make_shared<GPIOPin>(pin_number, direction);
    if (pin->initialize()) {
        pins_.push_back(pin);
        return pin;
    }
    
    return nullptr;
}

bool GPIOController::set_pin_value(int pin_number, GPIOValue value) {
    auto pin = find_pin(pin_number);
    if (pin) {
        return pin->set_value(value);
    }
    return false;
}

GPIOValue GPIOController::get_pin_value(int pin_number) const {
    auto pin = find_pin(pin_number);
    if (pin) {
        return pin->get_value();
    }
    return GPIOValue::LOW;
}

std::shared_ptr<GPIOPin> GPIOController::find_pin(int pin_number) const {
    auto it = std::find_if(pins_.begin(), pins_.end(),
        [pin_number](const std::shared_ptr<GPIOPin>& pin) {
            return pin->get_pin_number() == pin_number;
        });
    
    return (it != pins_.end()) ? *it : nullptr;
}

} // namespace hardware