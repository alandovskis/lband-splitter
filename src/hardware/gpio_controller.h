#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace splitter::hardware {

enum class PinDirection { INPUT, OUTPUT };

enum class PinState { LOW, HIGH };

class GpioController {
public:
  GpioController();
  ~GpioController();

  bool initialize();
  void cleanup();

  bool configure_input_pin(int pin);
  bool configure_output_pin(int pin);

  bool set_pin_high(int pin);
  bool set_pin_low(int pin);
  bool set_pin_state(int pin, PinState state);

  PinState get_pin_state(int pin) const;
  bool is_pin_configured(int pin) const;
  bool is_pin_healthy(int pin) const;

  bool is_healthy() const;
  std::string get_last_error() const;

private:
  struct PinInfo {
    PinDirection direction;
    PinState last_state;
    bool configured{false};
    bool healthy{true};
    std::string error_message;
  };

  bool export_pin(int pin);
  bool unexport_pin(int pin);
  bool set_pin_direction(int pin, PinDirection direction);
  bool write_pin_value(int pin, PinState state);
  PinState read_pin_value(int pin) const;

  bool write_to_sysfs(const std::string &path, const std::string &value);
  std::string read_from_sysfs(const std::string &path) const;

  std::string get_gpio_path(int pin, const std::string &file) const;

  std::unordered_map<int, PinInfo> pins_;
  mutable std::mutex pins_mutex_;

  bool initialized_{false};
  mutable std::string last_error_;

  static constexpr const char *GPIO_BASE_PATH = "/sys/class/gpio";
  static constexpr const char *GPIO_EXPORT_PATH = "/sys/class/gpio/export";
  static constexpr const char *GPIO_UNEXPORT_PATH = "/sys/class/gpio/unexport";
};

} // namespace splitter::hardware