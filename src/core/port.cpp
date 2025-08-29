#include "port.h"
#include "../hardware/stm32f4_controller.h"
#include "../utils/logger.h"

namespace splitter::core {

Port::Port(int id, hardware::STM32F4Controller *stm32f4)
    : id_(id), stm32f4_controller_(stm32f4),
      last_health_check_(std::chrono::steady_clock::now()) {

  state_.id = id;
  config_.name = "Port " + std::to_string(id + 1);
}

Port::~Port() {
  if (enabled_) {
    disable();
  }
}

bool Port::initialize() {
  if (!stm32f4_controller_) {
    set_error("Missing STM32F4 controller");
    return false;
  }

  // Initialize STM32F4 controller for this port
  if (!stm32f4_controller_->initialize()) {
    set_error("Failed to initialize STM32F4 controller");
    return false;
  }

  clear_error();
  healthy_ = true;

  utils::Logger::debug("Port {} initialized successfully", id_);
  return true;
}

bool Port::enable() {
  if (!stm32f4_controller_) {
    set_error("STM32F4 controller not available");
    return false;
  }

  // Send enable command to STM32F4 - it will handle LEDs autonomously
  if (!stm32f4_controller_->enable_port(true)) {
    set_error("Failed to enable port on STM32F4");
    return false;
  }

  enabled_ = true;
  state_.enabled = true;
  state_.last_update = std::chrono::system_clock::now();

  clear_error();

  utils::Logger::info("Port {} enabled", id_);
  return true;
}

bool Port::disable() {
  if (!stm32f4_controller_) {
    set_error("STM32F4 controller not available");
    return false;
  }

  // Send disable command to STM32F4 - it will handle LEDs autonomously
  if (!stm32f4_controller_->enable_port(false)) {
    set_error("Failed to disable port on STM32F4");
    return false;
  }

  enabled_ = false;
  signal_detected_ = false;
  state_.enabled = false;
  state_.signal_detected = false;
  state_.frequency_mhz = 0.0;
  state_.signal_level_dbm = -100.0;
  state_.last_update = std::chrono::system_clock::now();

  clear_error();

  utils::Logger::info("Port {} disabled", id_);
  return true;
}

bool Port::is_enabled() const { return enabled_; }

PortState Port::get_state() const {
  // Get the latest state from STM32F4 controller
  if (stm32f4_controller_) {
    auto reading = stm32f4_controller_->get_last_reading();
    state_.frequency_mhz = reading.frequency_mhz;
    state_.signal_level_dbm = reading.signal_level_dbm;
    state_.last_update = std::chrono::system_clock::now();
  }

  state_.enabled = enabled_;
  state_.signal_detected = signal_detected_;
  state_.healthy = healthy_;
  return state_;
}

PortConfig Port::get_configuration() const { return config_; }

bool Port::set_configuration(const PortConfig &config) {
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
    bool new_signal_detected = (frequency_mhz > 0.0) && in_range;
    
    if (signal_detected_ != new_signal_detected) {
      signal_detected_ = new_signal_detected;
      state_.signal_detected = signal_detected_;
      
      // Notify STM32F4 of signal detection change
      if (stm32f4_controller_) {
        stm32f4_controller_->set_signal_detection(signal_detected_);
      }
    }
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

  bool stm32f4_healthy =
      stm32f4_controller_ && stm32f4_controller_->is_healthy();

  bool was_healthy = healthy_;
  healthy_ = stm32f4_healthy;

  if (was_healthy && !healthy_) {
    set_error("STM32F4 controller health check failed");
    utils::Logger::warning("Port {} health check failed", id_);
  } else if (!was_healthy && healthy_) {
    clear_error();
    utils::Logger::info("Port {} health recovered", id_);
  }

  state_.healthy = healthy_;
}

void Port::set_error(const std::string &error) {
  state_.error_message = error;
  healthy_ = false;
  utils::Logger::error("Port {} error: {}", id_, error);
}

void Port::clear_error() { 
  state_.error_message.clear(); 
}

} // namespace splitter::core