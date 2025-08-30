#include "splitter_manager.h"
#include "../utils/logger.h"

#include <algorithm>
#include <chrono>

namespace splitter::core {

SplitterManager::SplitterManager(ConfigManager *config)
    : config_(config), last_frequency_update_(std::chrono::steady_clock::now()),
      last_health_check_(std::chrono::steady_clock::now()) {}

SplitterManager::~SplitterManager() {
  if (initialized_) {
    shutdown();
  }
}

bool SplitterManager::initialize() {
  if (initialized_) {
    return true;
  }

  try {
    // Initialize single STM32F4 controller for all 32 ports
    std::string uart_device = "/dev/ttyUSB0";  // Single UART for the MCU
    stm32f4_controller_ = std::make_unique<hardware::STM32F4Controller>(uart_device);
    
    if (!stm32f4_controller_->initialize()) {
      utils::Logger::error("Failed to initialize STM32F4 controller");
      return false;
    }

    // Load port configurations from config file
    auto port_configs = config_->get_port_configs();
    utils::Logger::info("Loading configuration for {} ports", port_configs.size());
    
    ports_.reserve(NUM_PORTS);
    for (int i = 0; i < NUM_PORTS; ++i) {
      auto port = std::make_unique<Port>(i, stm32f4_controller_.get());
      if (!port->initialize()) {
        utils::Logger::error("Failed to initialize port {}", i);
        return false;
      }
      
      // Apply configuration from config file if available
      if (i < static_cast<int>(port_configs.size()) && port_configs[i].is_object()) {
        auto config_json = port_configs[i];
        
        PortConfig port_config;
        port_config.name = config_json.value("name", "Port " + std::to_string(i + 1));
        port_config.signal_detection_enabled = config_json.value("signal_detection_enabled", true);
        
        bool enabled = config_json.value("enabled", false);
        
        if (!port->apply_startup_configuration(port_config, enabled)) {
          utils::Logger::error("Failed to apply startup configuration for port {}", i);
          return false;
        }
      } else {
        utils::Logger::debug("Using default configuration for port {}", i);
      }
      
      ports_.push_back(std::move(port));
    }

    initialized_ = true;
    utils::Logger::info(
        "SplitterManager initialized successfully with {} ports", NUM_PORTS);
    return true;

  } catch (const std::exception &e) {
    utils::Logger::error("Exception during initialization: {}", e.what());
    return false;
  }
}

void SplitterManager::shutdown() {
  if (!initialized_) {
    return;
  }

  shutdown_requested_ = true;

  std::lock_guard<std::mutex> lock(ports_mutex_);

  for (auto &port : ports_) {
    if (port) {
      port->disable();
    }
  }

  ports_.clear();
  stm32f4_controller_.reset();

  initialized_ = false;
  utils::Logger::info("SplitterManager shutdown complete");
}

bool SplitterManager::enable_port(int port_id) {
  if (port_id < 0 || port_id >= NUM_PORTS) {
    utils::Logger::warning("Invalid port ID: {}", port_id);
    return false;
  }

  std::lock_guard<std::mutex> lock(ports_mutex_);

  if (!ports_[port_id]) {
    utils::Logger::error("Port {} not initialized", port_id);
    return false;
  }

  if (ports_[port_id]->enable()) {
    auto state = ports_[port_id]->get_state();
    notify_state_change(port_id, state);
    utils::Logger::info("Port {} enabled", port_id);
    return true;
  }

  utils::Logger::error("Failed to enable port {}", port_id);
  return false;
}

bool SplitterManager::disable_port(int port_id) {
  if (port_id < 0 || port_id >= NUM_PORTS) {
    utils::Logger::warning("Invalid port ID: {}", port_id);
    return false;
  }

  std::lock_guard<std::mutex> lock(ports_mutex_);

  if (!ports_[port_id]) {
    utils::Logger::error("Port {} not initialized", port_id);
    return false;
  }

  if (ports_[port_id]->disable()) {
    auto state = ports_[port_id]->get_state();
    notify_state_change(port_id, state);
    utils::Logger::info("Port {} disabled", port_id);
    return true;
  }

  utils::Logger::error("Failed to disable port {}", port_id);
  return false;
}

bool SplitterManager::is_port_enabled(int port_id) const {
  if (port_id < 0 || port_id >= NUM_PORTS) {
    return false;
  }

  std::lock_guard<std::mutex> lock(ports_mutex_);
  return ports_[port_id] && ports_[port_id]->is_enabled();
}

PortState SplitterManager::get_port_state(int port_id) const {
  if (port_id < 0 || port_id >= NUM_PORTS) {
    return {};
  }

  std::lock_guard<std::mutex> lock(ports_mutex_);
  return ports_[port_id] ? ports_[port_id]->get_state() : PortState{};
}

std::vector<PortState> SplitterManager::get_all_port_states() const {
  std::vector<PortState> states;
  states.reserve(NUM_PORTS);

  std::lock_guard<std::mutex> lock(ports_mutex_);

  for (const auto &port : ports_) {
    states.push_back(port ? port->get_state() : PortState{});
  }

  return states;
}

double SplitterManager::get_port_frequency(int port_id) const {
  if (port_id < 0 || port_id >= NUM_PORTS) {
    return 0.0;
  }

  std::lock_guard<std::mutex> lock(ports_mutex_);
  return ports_[port_id] ? ports_[port_id]->get_state().frequency_mhz : 0.0;
}

bool SplitterManager::set_port_configuration(int port_id,
                                             const PortConfig &config) {
  if (port_id < 0 || port_id >= NUM_PORTS) {
    return false;
  }

  std::lock_guard<std::mutex> lock(ports_mutex_);

  if (!ports_[port_id]) {
    return false;
  }

  return ports_[port_id]->set_configuration(config);
}

PortConfig SplitterManager::get_port_configuration(int port_id) const {
  if (port_id < 0 || port_id >= NUM_PORTS) {
    return {};
  }

  std::lock_guard<std::mutex> lock(ports_mutex_);
  return ports_[port_id] ? ports_[port_id]->get_configuration() : PortConfig{};
}

void SplitterManager::register_state_change_callback(
    StateChangeCallback callback) {
  std::lock_guard<std::mutex> lock(callbacks_mutex_);
  state_callbacks_.push_back(std::move(callback));
}

void SplitterManager::register_frequency_change_callback(
    FrequencyChangeCallback callback) {
  std::lock_guard<std::mutex> lock(callbacks_mutex_);
  frequency_callbacks_.push_back(std::move(callback));
}

void SplitterManager::process_events() {
  if (!initialized_ || shutdown_requested_) {
    return;
  }

  auto now = std::chrono::steady_clock::now();

  if (now - last_frequency_update_ >= std::chrono::milliseconds(500)) {
    update_port_frequencies();
    last_frequency_update_ = now;
  }

  if (now - last_health_check_ >= std::chrono::seconds(5)) {
    check_port_health();
    last_health_check_ = now;
  }
}

bool SplitterManager::get_system_health() const {
  if (!initialized_) {
    return false;
  }

  // Check STM32F4 controllers health
  if (stm32f4_controller_ && !stm32f4_controller_->is_healthy()) {
    return false;
  }
  return true;
}

SystemStats SplitterManager::get_system_stats() const {
  SystemStats stats;
  stats.hardware_healthy = get_system_health();

  std::lock_guard<std::mutex> lock(ports_mutex_);

  uint32_t active_count = 0;
  double total_frequency = 0.0;

  for (int i = 0; i < NUM_PORTS; ++i) {
    if (ports_[i] && ports_[i]->is_enabled()) {
      ++active_count;
      total_frequency += get_port_frequency(i);
    }
  }

  stats.active_ports = active_count;
  stats.average_frequency_mhz =
      active_count > 0 ? total_frequency / active_count : 0.0;

  return stats;
}

void SplitterManager::update_port_frequencies() {
  std::lock_guard<std::mutex> lock(ports_mutex_);
  
  for (int i = 0; i < NUM_PORTS; ++i) {
    if (ports_[i] && ports_[i]->is_enabled()) {
      double frequency = ports_[i]->get_state().frequency_mhz;
      notify_frequency_change(i, frequency);
    }
  }
}

void SplitterManager::check_port_health() {
  std::lock_guard<std::mutex> lock(ports_mutex_);

  for (auto &port : ports_) {
    if (port) {
      port->check_health();
    }
  }
}

void SplitterManager::notify_state_change(int port_id, const PortState &state) {
  std::lock_guard<std::mutex> lock(callbacks_mutex_);

  for (const auto &callback : state_callbacks_) {
    try {
      callback(port_id, state);
    } catch (const std::exception &e) {
      utils::Logger::error("Exception in state change callback: {}", e.what());
    }
  }
}

void SplitterManager::notify_frequency_change(int port_id, double frequency) {
  std::lock_guard<std::mutex> lock(callbacks_mutex_);

  for (const auto &callback : frequency_callbacks_) {
    try {
      callback(port_id, frequency);
    } catch (const std::exception &e) {
      utils::Logger::error("Exception in frequency change callback: {}",
                           e.what());
    }
  }
}

} // namespace splitter::core