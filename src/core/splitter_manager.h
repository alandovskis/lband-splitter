#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "../hardware/frequency_detector.h"
#include "../hardware/gpio_controller.h"
#include "../hardware/stm32f4_controller.h"
#include "config_manager.h"
#include "port.h"

namespace splitter::core {

class SplitterManager {
public:
  using StateChangeCallback =
      std::function<void(int port_id, const PortState &state)>;
  using FrequencyChangeCallback =
      std::function<void(int port_id, double frequency_mhz)>;

  explicit SplitterManager(ConfigManager *config);
  ~SplitterManager();

  bool initialize();
  void shutdown();

  bool enable_port(int port_id);
  bool disable_port(int port_id);
  bool is_port_enabled(int port_id) const;

  PortState get_port_state(int port_id) const;
  std::vector<PortState> get_all_port_states() const;

  double get_port_frequency(int port_id) const;
  bool set_port_configuration(int port_id, const PortConfig &config);
  PortConfig get_port_configuration(int port_id) const;

  void register_state_change_callback(StateChangeCallback callback);
  void register_frequency_change_callback(FrequencyChangeCallback callback);

  void process_events();

  bool get_system_health() const;
  struct SystemStats get_system_stats() const;

  static constexpr int NUM_PORTS = 32;

private:
  void update_port_frequencies();
  void check_port_health();
  void notify_state_change(int port_id, const PortState &state);
  void notify_frequency_change(int port_id, double frequency);

  ConfigManager *config_;
  std::vector<std::unique_ptr<Port>> ports_;
  std::unique_ptr<hardware::GpioController> gpio_controller_;
  std::unique_ptr<hardware::FrequencyDetector> frequency_detector_;
  std::vector<std::unique_ptr<hardware::STM32F4Controller>> stm32f4_controllers_;

  mutable std::mutex ports_mutex_;
  std::atomic<bool> initialized_{false};
  std::atomic<bool> shutdown_requested_{false};

  std::vector<StateChangeCallback> state_callbacks_;
  std::vector<FrequencyChangeCallback> frequency_callbacks_;
  mutable std::mutex callbacks_mutex_;

  std::chrono::steady_clock::time_point last_frequency_update_;
  std::chrono::steady_clock::time_point last_health_check_;
};

struct SystemStats {
  std::chrono::system_clock::time_point uptime_start;
  uint64_t total_state_changes{0};
  uint64_t total_frequency_updates{0};
  uint32_t active_ports{0};
  double average_frequency_mhz{0.0};
  bool hardware_healthy{true};
};

} // namespace splitter::core