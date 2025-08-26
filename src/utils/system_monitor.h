#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace splitter::core {
class SplitterManager;
}

namespace splitter::utils {

struct SystemMetrics {
  double cpu_usage_percent{0.0};
  double memory_usage_percent{0.0};
  double disk_usage_percent{0.0};
  double temperature_celsius{0.0};
  uint64_t uptime_seconds{0};
  uint64_t network_rx_bytes{0};
  uint64_t network_tx_bytes{0};
  std::chrono::system_clock::time_point timestamp;

  SystemMetrics() : timestamp(std::chrono::system_clock::now()) {}
};

struct ApplicationMetrics {
  uint32_t active_ports{0};
  uint32_t total_state_changes{0};
  uint32_t total_frequency_updates{0};
  double average_frequency_mhz{0.0};
  bool hardware_healthy{true};
  std::chrono::system_clock::time_point last_update;

  ApplicationMetrics() : last_update(std::chrono::system_clock::now()) {}
};

class SystemMonitor {
public:
  explicit SystemMonitor(core::SplitterManager *splitter_manager);
  ~SystemMonitor();

  void start();
  void stop();

  SystemMetrics get_system_metrics() const;
  ApplicationMetrics get_application_metrics() const;

  void enable_telegraf_output(const std::string &socket_path);
  void disable_telegraf_output();

  bool is_running() const { return monitoring_active_; }
  std::string get_last_error() const { return last_error_; }

private:
  void monitoring_thread();
  void collect_system_metrics();
  void collect_application_metrics();
  void send_metrics_to_telegraf();

  bool read_proc_stat(double &cpu_usage);
  bool read_proc_meminfo(double &memory_usage);
  bool read_disk_usage(double &disk_usage);
  bool read_temperature(double &temperature);
  bool read_network_stats(uint64_t &rx_bytes, uint64_t &tx_bytes);
  uint64_t get_uptime_seconds();

  std::string format_telegraf_metric(
      const std::string &measurement,
      const std::unordered_map<std::string, std::string> &tags,
      const std::unordered_map<std::string, double> &fields,
      std::chrono::system_clock::time_point timestamp);

  core::SplitterManager *splitter_manager_;

  std::atomic<bool> monitoring_active_{false};
  std::unique_ptr<std::thread> monitoring_thread_;

  SystemMetrics system_metrics_;
  ApplicationMetrics app_metrics_;
  mutable std::mutex metrics_mutex_;

  std::string telegraf_socket_path_;
  bool telegraf_enabled_{false};
  int telegraf_socket_fd_{-1};

  std::chrono::steady_clock::time_point last_cpu_measurement_;
  uint64_t last_cpu_total_{0};
  uint64_t last_cpu_idle_{0};

  mutable std::string last_error_;

  static constexpr auto MONITORING_INTERVAL = std::chrono::seconds(30);
  static constexpr const char *PROC_STAT_PATH = "/proc/stat";
  static constexpr const char *PROC_MEMINFO_PATH = "/proc/meminfo";
  static constexpr const char *PROC_UPTIME_PATH = "/proc/uptime";
  static constexpr const char *THERMAL_ZONE_PATH =
      "/sys/class/thermal/thermal_zone0/temp";
  static constexpr const char *NETWORK_DEV_PATH = "/proc/net/dev";
};

} // namespace splitter::utils