#include "config_manager.h"
#include "../utils/logger.h"

#include <filesystem>
#include <fstream>

namespace splitter::core {

ConfigManager::ConfigManager(const std::string &config_file_path)
    : config_file_path_(config_file_path) {}

bool ConfigManager::load_config() {
  if (!std::filesystem::exists(config_file_path_)) {
    utils::Logger::warning("Config file {} not found, creating default",
                           config_file_path_);
    if (!create_default_config()) {
      return false;
    }
  }

  if (!load_from_file()) {
    return false;
  }

  merge_defaults();

  if (!validate_config()) {
    return false;
  }

  config_loaded_ = true;
  utils::Logger::info("Configuration loaded successfully from {}",
                      config_file_path_);
  return true;
}

bool ConfigManager::save_config() const {
  if (!config_loaded_) {
    utils::Logger::error("Cannot save config: not loaded");
    return false;
  }

  try {
    nlohmann::json config;

    config["hardware"] = {
        {"gpio_base_pin", hardware_config_.gpio_base_pin},
        {"status_led_base_pin", hardware_config_.status_led_base_pin},
        {"signal_led_base_pin", hardware_config_.signal_led_base_pin},
        {"spi_device", hardware_config_.spi_device},
        {"i2c_device", hardware_config_.i2c_device},
        {"frequency_detector_address",
         hardware_config_.frequency_detector_address}};

    config["network"] = {{"netconf_host", network_config_.netconf_host},
                         {"netconf_port", network_config_.netconf_port},
                         {"rest_host", network_config_.rest_host},
                         {"rest_port", network_config_.rest_port},
                         {"enable_ssl", network_config_.enable_ssl},
                         {"ssl_cert_path", network_config_.ssl_cert_path},
                         {"ssl_key_path", network_config_.ssl_key_path}};

    config["logging"] = {{"log_file", logging_config_.log_file},
                         {"log_level", logging_config_.log_level},
                         {"max_file_size_mb", logging_config_.max_file_size_mb},
                         {"max_files", logging_config_.max_files}};

    config["monitoring"] = {
        {"enable_telegraf", monitoring_config_.enable_telegraf},
        {"telegraf_socket", monitoring_config_.telegraf_socket},
        {"metrics_interval_seconds",
         monitoring_config_.metrics_interval_seconds},
        {"enable_health_endpoint", monitoring_config_.enable_health_endpoint}};

    config["ports"] = port_configs_;

    std::filesystem::create_directories(
        std::filesystem::path(config_file_path_).parent_path());

    std::ofstream file(config_file_path_);
    if (!file) {
      utils::Logger::error("Failed to open config file for writing: {}",
                           config_file_path_);
      return false;
    }

    file << config.dump(2);
    utils::Logger::info("Configuration saved to {}", config_file_path_);
    return true;

  } catch (const std::exception &e) {
    utils::Logger::error("Failed to save config: {}", e.what());
    return false;
  }
}

bool ConfigManager::validate_config() const {
  if (hardware_config_.gpio_base_pin < 0 ||
      hardware_config_.gpio_base_pin > 500) {
    utils::Logger::error("Invalid GPIO base pin: {}",
                         hardware_config_.gpio_base_pin);
    return false;
  }

  if (network_config_.netconf_port <= 0 ||
      network_config_.netconf_port > 65535) {
    utils::Logger::error("Invalid NetConf port: {}",
                         network_config_.netconf_port);
    return false;
  }

  if (network_config_.rest_port <= 0 || network_config_.rest_port > 65535) {
    utils::Logger::error("Invalid REST port: {}", network_config_.rest_port);
    return false;
  }

  if (network_config_.enable_ssl) {
    if (network_config_.ssl_cert_path.empty() ||
        network_config_.ssl_key_path.empty()) {
      utils::Logger::error("SSL enabled but certificate paths not specified");
      return false;
    }

    if (!std::filesystem::exists(network_config_.ssl_cert_path) ||
        !std::filesystem::exists(network_config_.ssl_key_path)) {
      utils::Logger::error("SSL certificate files not found");
      return false;
    }
  }

  if (logging_config_.max_file_size_mb <= 0) {
    utils::Logger::error("Invalid log file size: {}",
                         logging_config_.max_file_size_mb);
    return false;
  }

  return true;
}

bool ConfigManager::set_hardware_config(const HardwareConfig &config) {
  hardware_config_ = config;
  return validate_config();
}

bool ConfigManager::set_network_config(const NetworkConfig &config) {
  network_config_ = config;
  return validate_config();
}

bool ConfigManager::set_logging_config(const LoggingConfig &config) {
  logging_config_ = config;
  return validate_config();
}

bool ConfigManager::set_monitoring_config(const MonitoringConfig &config) {
  monitoring_config_ = config;
  return validate_config();
}

nlohmann::json ConfigManager::get_port_configs() const { return port_configs_; }

bool ConfigManager::set_port_configs(const nlohmann::json &configs) {
  try {
    if (!configs.is_array() || configs.size() != 32) {
      utils::Logger::error("Port configs must be array of 32 elements");
      return false;
    }

    port_configs_ = configs;
    return true;

  } catch (const std::exception &e) {
    utils::Logger::error("Failed to set port configs: {}", e.what());
    return false;
  }
}

std::optional<std::string> ConfigManager::get_yang_model_path() const {
  if (std::filesystem::exists(DEFAULT_YANG_MODEL)) {
    return DEFAULT_YANG_MODEL;
  }

  std::string local_model = "config/yang/splitter.yang";
  if (std::filesystem::exists(local_model)) {
    return local_model;
  }

  return std::nullopt;
}

std::vector<std::string> ConfigManager::get_yang_search_paths() const {
  return {"/usr/share/splitter/yang", "/usr/local/share/splitter/yang",
          "config/yang", "."};
}

bool ConfigManager::load_from_file() {
  try {
    std::ifstream file(config_file_path_);
    if (!file) {
      utils::Logger::error("Failed to open config file: {}", config_file_path_);
      return false;
    }

    file >> raw_config_;

    if (raw_config_.contains("hardware")) {
      auto hw = raw_config_["hardware"];
      hardware_config_.gpio_base_pin =
          hw.value("gpio_base_pin", hardware_config_.gpio_base_pin);
      hardware_config_.status_led_base_pin =
          hw.value("status_led_base_pin", hardware_config_.status_led_base_pin);
      hardware_config_.signal_led_base_pin =
          hw.value("signal_led_base_pin", hardware_config_.signal_led_base_pin);
      hardware_config_.spi_device =
          hw.value("spi_device", hardware_config_.spi_device);
      hardware_config_.i2c_device =
          hw.value("i2c_device", hardware_config_.i2c_device);
      hardware_config_.frequency_detector_address =
          hw.value("frequency_detector_address",
                   hardware_config_.frequency_detector_address);
    }

    if (raw_config_.contains("network")) {
      auto net = raw_config_["network"];
      network_config_.netconf_host =
          net.value("netconf_host", network_config_.netconf_host);
      network_config_.netconf_port =
          net.value("netconf_port", network_config_.netconf_port);
      network_config_.rest_host =
          net.value("rest_host", network_config_.rest_host);
      network_config_.rest_port =
          net.value("rest_port", network_config_.rest_port);
      network_config_.enable_ssl =
          net.value("enable_ssl", network_config_.enable_ssl);
      network_config_.ssl_cert_path =
          net.value("ssl_cert_path", network_config_.ssl_cert_path);
      network_config_.ssl_key_path =
          net.value("ssl_key_path", network_config_.ssl_key_path);
    }

    if (raw_config_.contains("logging")) {
      auto log = raw_config_["logging"];
      logging_config_.log_file =
          log.value("log_file", logging_config_.log_file);
      logging_config_.log_level =
          log.value("log_level", logging_config_.log_level);
      logging_config_.max_file_size_mb =
          log.value("max_file_size_mb", logging_config_.max_file_size_mb);
      logging_config_.max_files =
          log.value("max_files", logging_config_.max_files);
    }

    if (raw_config_.contains("monitoring")) {
      auto mon = raw_config_["monitoring"];
      monitoring_config_.enable_telegraf =
          mon.value("enable_telegraf", monitoring_config_.enable_telegraf);
      monitoring_config_.telegraf_socket =
          mon.value("telegraf_socket", monitoring_config_.telegraf_socket);
      monitoring_config_.metrics_interval_seconds =
          mon.value("metrics_interval_seconds",
                    monitoring_config_.metrics_interval_seconds);
      monitoring_config_.enable_health_endpoint = mon.value(
          "enable_health_endpoint", monitoring_config_.enable_health_endpoint);
    }

    if (raw_config_.contains("ports")) {
      port_configs_ = raw_config_["ports"];
    }

    return true;

  } catch (const std::exception &e) {
    utils::Logger::error("Failed to parse config file: {}", e.what());
    return false;
  }
}

bool ConfigManager::create_default_config() {
  try {
    port_configs_ = nlohmann::json::array();
    for (int i = 0; i < 32; ++i) {
      port_configs_.push_back({{"id", i},
                               {"name", "Port " + std::to_string(i + 1)},
                               {"enabled", false},
                               {"auto_enable", false},
                               {"min_frequency_mhz", 1000.0},
                               {"max_frequency_mhz", 2000.0},
                               {"gain_db", 0},
                               {"signal_detection_enabled", true}});
    }

    // Create default config JSON structure
    nlohmann::json config;

    config["hardware"] = {
        {"gpio_base_pin", hardware_config_.gpio_base_pin},
        {"status_led_base_pin", hardware_config_.status_led_base_pin},
        {"signal_led_base_pin", hardware_config_.signal_led_base_pin},
        {"spi_device", hardware_config_.spi_device},
        {"i2c_device", hardware_config_.i2c_device},
        {"frequency_detector_address",
         hardware_config_.frequency_detector_address}};

    config["network"] = {{"netconf_host", network_config_.netconf_host},
                         {"netconf_port", network_config_.netconf_port},
                         {"rest_host", network_config_.rest_host},
                         {"rest_port", network_config_.rest_port},
                         {"enable_ssl", network_config_.enable_ssl},
                         {"ssl_cert_path", network_config_.ssl_cert_path},
                         {"ssl_key_path", network_config_.ssl_key_path}};

    config["logging"] = {{"log_file", logging_config_.log_file},
                         {"log_level", logging_config_.log_level},
                         {"max_file_size_mb", logging_config_.max_file_size_mb},
                         {"max_files", logging_config_.max_files}};

    config["monitoring"] = {
        {"enable_telegraf", monitoring_config_.enable_telegraf},
        {"telegraf_socket", monitoring_config_.telegraf_socket},
        {"metrics_interval_seconds",
         monitoring_config_.metrics_interval_seconds},
        {"enable_health_endpoint", monitoring_config_.enable_health_endpoint}};

    config["ports"] = port_configs_;

    std::filesystem::create_directories(
        std::filesystem::path(config_file_path_).parent_path());

    std::ofstream file(config_file_path_);
    if (!file) {
      utils::Logger::error("Failed to open config file for writing: {}",
                           config_file_path_);
      return false;
    }

    file << config.dump(2);
    utils::Logger::info("Default configuration created at {}", config_file_path_);
    return true;

  } catch (const std::exception &e) {
    utils::Logger::error("Failed to create default config: {}", e.what());
    return false;
  }
}

void ConfigManager::merge_defaults() {}

} // namespace splitter::core