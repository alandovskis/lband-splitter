#include "system_monitor.h"
#include "../core/splitter_manager.h"
#include "logger.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <sys/statvfs.h>
#include <sys/un.h>
#include <unistd.h>

namespace splitter::utils {

SystemMonitor::SystemMonitor(core::SplitterManager *splitter_manager)
    : splitter_manager_(splitter_manager),
      last_cpu_measurement_(std::chrono::steady_clock::now()) {}

SystemMonitor::~SystemMonitor() { stop(); }

void SystemMonitor::start() {
  if (monitoring_active_) {
    return;
  }

  monitoring_active_ = true;
  monitoring_thread_ =
      std::make_unique<std::thread>(&SystemMonitor::monitoring_thread, this);

  Logger::info("System monitor started");
}

void SystemMonitor::stop() {
  if (!monitoring_active_) {
    return;
  }

  monitoring_active_ = false;

  if (monitoring_thread_ && monitoring_thread_->joinable()) {
    monitoring_thread_->join();
  }

  monitoring_thread_.reset();

  if (telegraf_socket_fd_ >= 0) {
    close(telegraf_socket_fd_);
    telegraf_socket_fd_ = -1;
  }

  Logger::info("System monitor stopped");
}

SystemMetrics SystemMonitor::get_system_metrics() const {
  std::lock_guard<std::mutex> lock(metrics_mutex_);
  return system_metrics_;
}

ApplicationMetrics SystemMonitor::get_application_metrics() const {
  std::lock_guard<std::mutex> lock(metrics_mutex_);
  return app_metrics_;
}

void SystemMonitor::enable_telegraf_output(const std::string &socket_path) {
  telegraf_socket_path_ = socket_path;
  telegraf_enabled_ = true;

  Logger::info("Telegraf output enabled: {}", socket_path);
}

void SystemMonitor::disable_telegraf_output() {
  telegraf_enabled_ = false;

  if (telegraf_socket_fd_ >= 0) {
    close(telegraf_socket_fd_);
    telegraf_socket_fd_ = -1;
  }

  Logger::info("Telegraf output disabled");
}

void SystemMonitor::monitoring_thread() {
  Logger::debug("System monitoring thread started");

  while (monitoring_active_) {
    try {
      collect_system_metrics();
      collect_application_metrics();

      if (telegraf_enabled_) {
        send_metrics_to_telegraf();
      }

    } catch (const std::exception &e) {
      Logger::error("Exception in monitoring thread: {}", e.what());
      last_error_ = e.what();
    }

    std::this_thread::sleep_for(MONITORING_INTERVAL);
  }

  Logger::debug("System monitoring thread stopped");
}

void SystemMonitor::collect_system_metrics() {
  SystemMetrics metrics;

  read_proc_stat(metrics.cpu_usage_percent);
  read_proc_meminfo(metrics.memory_usage_percent);
  read_disk_usage(metrics.disk_usage_percent);
  read_temperature(metrics.temperature_celsius);
  read_network_stats(metrics.network_rx_bytes, metrics.network_tx_bytes);
  metrics.uptime_seconds = get_uptime_seconds();
  metrics.timestamp = std::chrono::system_clock::now();

  std::lock_guard<std::mutex> lock(metrics_mutex_);
  system_metrics_ = metrics;
}

void SystemMonitor::collect_application_metrics() {
  if (!splitter_manager_) {
    return;
  }

  ApplicationMetrics metrics;
  auto stats = splitter_manager_->get_system_stats();

  metrics.active_ports = stats.active_ports;
  metrics.total_state_changes = stats.total_state_changes;
  metrics.total_frequency_updates = stats.total_frequency_updates;
  metrics.average_frequency_mhz = stats.average_frequency_mhz;
  metrics.hardware_healthy = stats.hardware_healthy;
  metrics.last_update = std::chrono::system_clock::now();

  std::lock_guard<std::mutex> lock(metrics_mutex_);
  app_metrics_ = metrics;
}

void SystemMonitor::send_metrics_to_telegraf() {
  if (!telegraf_enabled_ || telegraf_socket_path_.empty()) {
    return;
  }

  if (telegraf_socket_fd_ < 0) {
    telegraf_socket_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (telegraf_socket_fd_ < 0) {
      last_error_ = "Failed to create telegraf socket";
      return;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, telegraf_socket_path_.c_str(),
            sizeof(addr.sun_path) - 1);

    if (connect(telegraf_socket_fd_, (struct sockaddr *)&addr, sizeof(addr)) <
        0) {
      close(telegraf_socket_fd_);
      telegraf_socket_fd_ = -1;
      last_error_ = "Failed to connect to telegraf socket";
      return;
    }
  }

  try {
    std::lock_guard<std::mutex> lock(metrics_mutex_);

    auto now = std::chrono::system_clock::now();
    std::unordered_map<std::string, std::string> tags{{"host", "splitter"}};

    std::unordered_map<std::string, double> system_fields{
        {"cpu_usage", system_metrics_.cpu_usage_percent},
        {"memory_usage", system_metrics_.memory_usage_percent},
        {"disk_usage", system_metrics_.disk_usage_percent},
        {"temperature", system_metrics_.temperature_celsius},
        {"uptime", static_cast<double>(system_metrics_.uptime_seconds)},
        {"network_rx", static_cast<double>(system_metrics_.network_rx_bytes)},
        {"network_tx", static_cast<double>(system_metrics_.network_tx_bytes)}};

    std::string system_metric =
        format_telegraf_metric("system", tags, system_fields, now);

    std::unordered_map<std::string, double> app_fields{
        {"active_ports", static_cast<double>(app_metrics_.active_ports)},
        {"state_changes",
         static_cast<double>(app_metrics_.total_state_changes)},
        {"frequency_updates",
         static_cast<double>(app_metrics_.total_frequency_updates)},
        {"avg_frequency", app_metrics_.average_frequency_mhz},
        {"hardware_healthy", app_metrics_.hardware_healthy ? 1.0 : 0.0}};

    std::string app_metric =
        format_telegraf_metric("splitter", tags, app_fields, now);

    std::string payload = system_metric + "\n" + app_metric + "\n";
    send(telegraf_socket_fd_, payload.c_str(), payload.length(), MSG_NOSIGNAL);

  } catch (const std::exception &e) {
    Logger::warning("Failed to send metrics to telegraf: {}", e.what());
    if (telegraf_socket_fd_ >= 0) {
      close(telegraf_socket_fd_);
      telegraf_socket_fd_ = -1;
    }
  }
}

bool SystemMonitor::read_proc_stat(double &cpu_usage) {
  try {
    std::ifstream file(PROC_STAT_PATH);
    if (!file) {
      return false;
    }

    std::string line;
    if (!std::getline(file, line)) {
      return false;
    }

    std::istringstream iss(line);
    std::string cpu_label;
    uint64_t user, nice, system, idle, iowait, irq, softirq, steal;

    if (!(iss >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >>
          softirq >> steal)) {
      return false;
    }

    uint64_t total =
        user + nice + system + idle + iowait + irq + softirq + steal;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = now - last_cpu_measurement_;

    if (last_cpu_total_ > 0 && elapsed > std::chrono::seconds(1)) {
      uint64_t total_diff = total - last_cpu_total_;
      uint64_t idle_diff = idle - last_cpu_idle_;

      if (total_diff > 0) {
        cpu_usage = 100.0 * (1.0 - static_cast<double>(idle_diff) / total_diff);
      }
    }

    last_cpu_total_ = total;
    last_cpu_idle_ = idle;
    last_cpu_measurement_ = now;

    return true;

  } catch (const std::exception &e) {
    Logger::warning("Failed to read CPU stats: {}", e.what());
    return false;
  }
}

bool SystemMonitor::read_proc_meminfo(double &memory_usage) {
  try {
    std::ifstream file(PROC_MEMINFO_PATH);
    if (!file) {
      return false;
    }

    uint64_t total_kb = 0, available_kb = 0;
    std::string line;

    while (std::getline(file, line)) {
      std::istringstream iss(line);
      std::string key;
      uint64_t value;
      std::string unit;

      if (iss >> key >> value >> unit) {
        if (key == "MemTotal:") {
          total_kb = value;
        } else if (key == "MemAvailable:") {
          available_kb = value;
        }
      }
    }

    if (total_kb > 0 && available_kb > 0) {
      memory_usage =
          100.0 * (1.0 - static_cast<double>(available_kb) / total_kb);
      return true;
    }

    return false;

  } catch (const std::exception &e) {
    Logger::warning("Failed to read memory stats: {}", e.what());
    return false;
  }
}

bool SystemMonitor::read_disk_usage(double &disk_usage) {
  try {
    struct statvfs stat;
    if (statvfs("/", &stat) == 0) {
      uint64_t total = stat.f_blocks * stat.f_frsize;
      uint64_t available = stat.f_bavail * stat.f_frsize;
      uint64_t used = total - available;

      if (total > 0) {
        disk_usage = 100.0 * static_cast<double>(used) / total;
        return true;
      }
    }

    return false;

  } catch (const std::exception &e) {
    Logger::warning("Failed to read disk stats: {}", e.what());
    return false;
  }
}

bool SystemMonitor::read_temperature(double &temperature) {
  try {
    if (!std::filesystem::exists(THERMAL_ZONE_PATH)) {
      temperature = 0.0;
      return true;
    }

    std::ifstream file(THERMAL_ZONE_PATH);
    if (!file) {
      return false;
    }

    int temp_millidegrees;
    if (file >> temp_millidegrees) {
      temperature = temp_millidegrees / 1000.0;
      return true;
    }

    return false;

  } catch (const std::exception &e) {
    Logger::warning("Failed to read temperature: {}", e.what());
    return false;
  }
}

bool SystemMonitor::read_network_stats(uint64_t &rx_bytes, uint64_t &tx_bytes) {
  try {
    std::ifstream file(NETWORK_DEV_PATH);
    if (!file) {
      return false;
    }

    std::string line;
    std::getline(file, line);
    std::getline(file, line);

    rx_bytes = 0;
    tx_bytes = 0;

    while (std::getline(file, line)) {
      size_t colon_pos = line.find(':');
      if (colon_pos == std::string::npos) {
        continue;
      }

      std::string interface = line.substr(0, colon_pos);
      interface.erase(0, interface.find_first_not_of(" \t"));
      interface.erase(interface.find_last_not_of(" \t") + 1);

      if (interface == "lo") {
        continue;
      }

      std::istringstream iss(line.substr(colon_pos + 1));
      uint64_t rx, tx;
      for (int i = 0; i < 8; ++i) {
        if (i == 0) {
          iss >> rx;
        } else {
          uint64_t dummy;
          iss >> dummy;
        }
      }
      iss >> tx;

      rx_bytes += rx;
      tx_bytes += tx;
    }

    return true;

  } catch (const std::exception &e) {
    Logger::warning("Failed to read network stats: {}", e.what());
    return false;
  }
}

uint64_t SystemMonitor::get_uptime_seconds() {
  try {
    std::ifstream file(PROC_UPTIME_PATH);
    if (!file) {
      return 0;
    }

    double uptime;
    if (file >> uptime) {
      return static_cast<uint64_t>(uptime);
    }

    return 0;

  } catch (const std::exception &e) {
    Logger::warning("Failed to read uptime: {}", e.what());
    return 0;
  }
}

std::string SystemMonitor::format_telegraf_metric(
    const std::string &measurement,
    const std::unordered_map<std::string, std::string> &tags,
    const std::unordered_map<std::string, double> &fields,
    std::chrono::system_clock::time_point timestamp) {

  std::ostringstream oss;
  oss << measurement;

  for (const auto &[key, value] : tags) {
    oss << "," << key << "=" << value;
  }

  oss << " ";

  bool first_field = true;
  for (const auto &[key, value] : fields) {
    if (!first_field) {
      oss << ",";
    }
    oss << key << "=" << value;
    first_field = false;
  }

  auto epoch = timestamp.time_since_epoch();
  auto nanoseconds =
      std::chrono::duration_cast<std::chrono::nanoseconds>(epoch);
  oss << " " << nanoseconds.count();

  return oss.str();
}

} // namespace splitter::utils