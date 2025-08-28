#pragma once

#include <atomic>
#include <chrono>
#include <string>

namespace splitter::hardware {
class GpioController;
class STM32F4Controller;
} // namespace splitter::hardware

namespace splitter::core {

struct PortConfig {
  std::string name;
  bool auto_enable{false};
  double min_frequency_mhz{1000.0};
  double max_frequency_mhz{2000.0};
  int gain_db{0};
  bool signal_detection_enabled{true};
};

struct PortState {
  int id{-1};
  bool enabled{false};
  bool signal_detected{false};
  bool healthy{true};
  double frequency_mhz{0.0};
  double signal_level_dbm{-100.0};
  std::chrono::system_clock::time_point last_update;
  std::string error_message;

  PortState() : last_update(std::chrono::system_clock::now()) {}
};

class Port {
public:
  Port(int id, hardware::GpioController *gpio,
       hardware::STM32F4Controller *stm32f4);
  ~Port();

  bool initialize();

  bool enable();
  bool disable();
  bool is_enabled() const;

  PortState get_state() const;
  PortConfig get_configuration() const;
  bool set_configuration(const PortConfig &config);

  void update_frequency(double frequency_mhz);
  void update_signal_level(double level_dbm);

  void check_health();

private:
  void update_port_state();
  void update_display();
  void set_error(const std::string &error);
  void clear_error();

  int id_;
  hardware::GpioController *gpio_controller_;
  hardware::STM32F4Controller *stm32f4_controller_;

  PortConfig config_;
  mutable PortState state_;

  std::atomic<bool> enabled_{false};
  std::atomic<bool> signal_detected_{false};
  std::atomic<bool> healthy_{true};

  int enable_gpio_pin_;

  std::chrono::steady_clock::time_point last_health_check_;

  static constexpr int GPIO_BASE_PIN = 100;
};

} // namespace splitter::core