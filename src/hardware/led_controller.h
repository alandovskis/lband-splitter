#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace splitter::hardware {

class GpioController;

enum class LedState { OFF, ON, BLINKING_SLOW, BLINKING_FAST };

struct LedInfo {
  int gpio_pin{-1};
  LedState state{LedState::OFF};
  bool configured{false};
  bool healthy{true};
  std::chrono::steady_clock::time_point last_toggle;
  bool current_physical_state{false};
  std::string error_message;
};

class LedController {
public:
  explicit LedController(GpioController *gpio_controller);
  ~LedController();

  bool initialize();
  void cleanup();

  bool configure_led(int led_id);
  bool configure_led_with_pin(int led_id, int gpio_pin);

  bool set_led_state(int led_id, LedState state);
  bool set_led_on(int led_id);
  bool set_led_off(int led_id);
  bool set_led_blinking_slow(int led_id);
  bool set_led_blinking_fast(int led_id);

  LedState get_led_state(int led_id) const;
  bool is_led_configured(int led_id) const;
  bool is_led_healthy(int led_id) const;

  void start_blink_thread();
  void stop_blink_thread();

  bool is_healthy() const;
  std::string get_last_error() const;

private:
  void blink_thread_function();
  void update_blinking_leds();
  bool should_toggle_led(const LedInfo &led_info,
                         std::chrono::steady_clock::time_point now) const;
  std::chrono::milliseconds get_blink_interval(LedState state) const;

  GpioController *gpio_controller_;

  std::unordered_map<int, LedInfo> leds_;
  mutable std::mutex leds_mutex_;

  std::atomic<bool> initialized_{false};
  std::atomic<bool> blink_thread_running_{false};
  std::unique_ptr<std::thread> blink_thread_;

  mutable std::string last_error_;

  static constexpr auto SLOW_BLINK_INTERVAL = std::chrono::milliseconds(1000);
  static constexpr auto FAST_BLINK_INTERVAL = std::chrono::milliseconds(250);
  static constexpr auto BLINK_THREAD_INTERVAL = std::chrono::milliseconds(50);
};

} // namespace splitter::hardware