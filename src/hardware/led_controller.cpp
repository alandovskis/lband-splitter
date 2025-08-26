#include "led_controller.h"
#include "../utils/logger.h"
#include "gpio_controller.h"

namespace splitter::hardware {

LedController::LedController(GpioController *gpio_controller)
    : gpio_controller_(gpio_controller) {}

LedController::~LedController() { cleanup(); }

bool LedController::initialize() {
  if (initialized_) {
    return true;
  }

  if (!gpio_controller_) {
    last_error_ = "GPIO controller not provided";
    return false;
  }

  start_blink_thread();

  initialized_ = true;
  utils::Logger::info("LED controller initialized");
  return true;
}

void LedController::cleanup() {
  if (!initialized_) {
    return;
  }

  stop_blink_thread();

  std::lock_guard<std::mutex> lock(leds_mutex_);

  for (auto &[led_id, led_info] : leds_) {
    if (led_info.configured) {
      set_led_state(led_id, LedState::OFF);
    }
  }

  leds_.clear();
  initialized_ = false;

  utils::Logger::info("LED controller cleanup complete");
}

bool LedController::configure_led(int led_id) {
  return configure_led_with_pin(led_id, led_id);
}

bool LedController::configure_led_with_pin(int led_id, int gpio_pin) {
  if (!gpio_controller_) {
    last_error_ = "GPIO controller not available";
    return false;
  }

  std::lock_guard<std::mutex> lock(leds_mutex_);

  if (!gpio_controller_->configure_output_pin(gpio_pin)) {
    last_error_ = "Failed to configure GPIO pin " + std::to_string(gpio_pin) +
                  " for LED " + std::to_string(led_id);
    return false;
  }

  LedInfo led_info;
  led_info.gpio_pin = gpio_pin;
  led_info.configured = true;
  led_info.healthy = true;
  led_info.state = LedState::OFF;
  led_info.last_toggle = std::chrono::steady_clock::now();
  led_info.current_physical_state = false;

  leds_[led_id] = led_info;

  gpio_controller_->set_pin_low(gpio_pin);

  utils::Logger::debug("Configured LED {} on GPIO pin {}", led_id, gpio_pin);
  return true;
}

bool LedController::set_led_state(int led_id, LedState state) {
  std::lock_guard<std::mutex> lock(leds_mutex_);

  auto it = leds_.find(led_id);
  if (it == leds_.end() || !it->second.configured) {
    last_error_ = "LED " + std::to_string(led_id) + " not configured";
    return false;
  }

  it->second.state = state;
  it->second.last_toggle = std::chrono::steady_clock::now();

  bool physical_state = false;
  switch (state) {
  case LedState::ON:
    physical_state = true;
    break;
  case LedState::OFF:
    physical_state = false;
    break;
  case LedState::BLINKING_SLOW:
  case LedState::BLINKING_FAST:
    physical_state = true;
    break;
  }

  if (state == LedState::ON || state == LedState::OFF) {
    if (physical_state) {
      if (!gpio_controller_->set_pin_high(it->second.gpio_pin)) {
        it->second.healthy = false;
        it->second.error_message = "Failed to turn LED on";
        return false;
      }
    } else {
      if (!gpio_controller_->set_pin_low(it->second.gpio_pin)) {
        it->second.healthy = false;
        it->second.error_message = "Failed to turn LED off";
        return false;
      }
    }
  }

  it->second.current_physical_state = physical_state;
  it->second.healthy = true;
  it->second.error_message.clear();

  return true;
}

bool LedController::set_led_on(int led_id) {
  return set_led_state(led_id, LedState::ON);
}

bool LedController::set_led_off(int led_id) {
  return set_led_state(led_id, LedState::OFF);
}

bool LedController::set_led_blinking_slow(int led_id) {
  return set_led_state(led_id, LedState::BLINKING_SLOW);
}

bool LedController::set_led_blinking_fast(int led_id) {
  return set_led_state(led_id, LedState::BLINKING_FAST);
}

LedState LedController::get_led_state(int led_id) const {
  std::lock_guard<std::mutex> lock(leds_mutex_);

  auto it = leds_.find(led_id);
  if (it == leds_.end()) {
    return LedState::OFF;
  }

  return it->second.state;
}

bool LedController::is_led_configured(int led_id) const {
  std::lock_guard<std::mutex> lock(leds_mutex_);

  auto it = leds_.find(led_id);
  return it != leds_.end() && it->second.configured;
}

bool LedController::is_led_healthy(int led_id) const {
  std::lock_guard<std::mutex> lock(leds_mutex_);

  auto it = leds_.find(led_id);
  return it != leds_.end() && it->second.healthy;
}

void LedController::start_blink_thread() {
  if (blink_thread_running_) {
    return;
  }

  blink_thread_running_ = true;
  blink_thread_ = std::make_unique<std::thread>(
      &LedController::blink_thread_function, this);

  utils::Logger::debug("LED blink thread started");
}

void LedController::stop_blink_thread() {
  if (!blink_thread_running_) {
    return;
  }

  blink_thread_running_ = false;

  if (blink_thread_ && blink_thread_->joinable()) {
    blink_thread_->join();
  }

  blink_thread_.reset();
  utils::Logger::debug("LED blink thread stopped");
}

bool LedController::is_healthy() const {
  if (!initialized_ || !gpio_controller_) {
    return false;
  }

  std::lock_guard<std::mutex> lock(leds_mutex_);

  for (const auto &[led_id, led_info] : leds_) {
    if (led_info.configured && !led_info.healthy) {
      return false;
    }
  }

  return true;
}

std::string LedController::get_last_error() const { return last_error_; }

void LedController::blink_thread_function() {
  utils::Logger::debug("LED blink thread running");

  while (blink_thread_running_) {
    update_blinking_leds();
    std::this_thread::sleep_for(BLINK_THREAD_INTERVAL);
  }

  utils::Logger::debug("LED blink thread exiting");
}

void LedController::update_blinking_leds() {
  if (!gpio_controller_) {
    return;
  }

  std::lock_guard<std::mutex> lock(leds_mutex_);
  auto now = std::chrono::steady_clock::now();

  for (auto &[led_id, led_info] : leds_) {
    if (!led_info.configured || !led_info.healthy) {
      continue;
    }

    if (led_info.state == LedState::BLINKING_SLOW ||
        led_info.state == LedState::BLINKING_FAST) {
      if (should_toggle_led(led_info, now)) {
        bool new_state = !led_info.current_physical_state;

        bool success = new_state
                           ? gpio_controller_->set_pin_high(led_info.gpio_pin)
                           : gpio_controller_->set_pin_low(led_info.gpio_pin);

        if (success) {
          led_info.current_physical_state = new_state;
          led_info.last_toggle = now;
          led_info.healthy = true;
          led_info.error_message.clear();
        } else {
          led_info.healthy = false;
          led_info.error_message = "Failed to toggle LED during blinking";
          utils::Logger::warning("Failed to toggle LED {} during blinking",
                                 led_id);
        }
      }
    }
  }
}

bool LedController::should_toggle_led(
    const LedInfo &led_info, std::chrono::steady_clock::time_point now) const {
  auto elapsed = now - led_info.last_toggle;
  auto required_interval = get_blink_interval(led_info.state);

  return elapsed >= required_interval;
}

std::chrono::milliseconds
LedController::get_blink_interval(LedState state) const {
  switch (state) {
  case LedState::BLINKING_SLOW:
    return SLOW_BLINK_INTERVAL;
  case LedState::BLINKING_FAST:
    return FAST_BLINK_INTERVAL;
  default:
    return std::chrono::milliseconds(0);
  }
}

} // namespace splitter::hardware