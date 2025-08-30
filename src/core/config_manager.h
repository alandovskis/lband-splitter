#pragma once

#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace splitter::core {

// Hardware config removed - STM32 MCU handles all hardware directly

struct NetworkConfig {
  std::string netconf_host{"0.0.0.0"};
  int netconf_port{830};
  std::string rest_host{"127.0.0.1"};
  int rest_port{8080};
  bool enable_ssl{false};
  std::string ssl_cert_path;
  std::string ssl_key_path;
};

struct LoggingConfig {
  std::string log_file{"/var/log/splitter/daemon.log"};
  std::string log_level{"info"};
  int max_file_size_mb{100};
  int max_files{10};
};

struct MonitoringConfig {
  int metrics_interval_seconds{30};
  bool enable_health_endpoint{true};
};

class ConfigManager {
public:
  explicit ConfigManager(const std::string &config_file_path);

  bool load_config();
  bool save_config() const;
  bool validate_config() const;

  // Hardware config removed - STM32 handles all hardware
  const NetworkConfig &get_network_config() const { return network_config_; }
  const LoggingConfig &get_logging_config() const { return logging_config_; }
  const MonitoringConfig &get_monitoring_config() const {
    return monitoring_config_;
  }

  int get_web_port() const { return network_config_.rest_port; }
  std::string get_log_file() const { return logging_config_.log_file; }

  // Hardware config removed
  bool set_network_config(const NetworkConfig &config);
  bool set_logging_config(const LoggingConfig &config);
  bool set_monitoring_config(const MonitoringConfig &config);

  nlohmann::json get_port_configs() const;
  bool set_port_configs(const nlohmann::json &configs);

  std::optional<std::string> get_yang_model_path() const;
  std::vector<std::string> get_yang_search_paths() const;

private:
  bool load_from_file();
  bool create_default_config();
  void merge_defaults();
  nlohmann::json build_config_json() const;

  std::string config_file_path_;

  // Hardware config removed
  NetworkConfig network_config_;
  LoggingConfig logging_config_;
  MonitoringConfig monitoring_config_;

  nlohmann::json port_configs_;
  nlohmann::json raw_config_;

  bool config_loaded_{false};

  static constexpr const char *DEFAULT_YANG_MODEL =
      "/usr/share/splitter/yang/splitter.yang";
};

} // namespace splitter::core