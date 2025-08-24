#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace hardware {

enum class GPIODirection {
    INPUT,
    OUTPUT
};

enum class GPIOValue {
    LOW = 0,
    HIGH = 1
};

class GPIOPin {
public:
    GPIOPin(int pin_number, GPIODirection direction);
    ~GPIOPin();

    bool initialize();
    bool set_value(GPIOValue value);
    GPIOValue get_value() const;
    bool is_initialized() const { return initialized_; }
    int get_pin_number() const { return pin_number_; }

private:
    int pin_number_;
    GPIODirection direction_;
    bool initialized_;
    mutable std::mutex mutex_;
    
    bool export_pin();
    bool unexport_pin();
    bool set_direction();
};

class GPIOController {
public:
    GPIOController();
    ~GPIOController();

    bool initialize();
    std::shared_ptr<GPIOPin> get_pin(int pin_number, GPIODirection direction);
    bool set_pin_value(int pin_number, GPIOValue value);
    GPIOValue get_pin_value(int pin_number) const;

private:
    std::vector<std::shared_ptr<GPIOPin>> pins_;
    mutable std::mutex mutex_;
    bool initialized_;
    
    std::shared_ptr<GPIOPin> find_pin(int pin_number) const;
};

} // namespace hardware