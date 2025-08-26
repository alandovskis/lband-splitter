#pragma once

#include <string>
#include <vector>
#include <optional>

#include <nlohmann/json.hpp>

namespace splitter::core {

struct HardwareConfig {
    int gpio_base_pin{100};
    int status_led_base_pin{200};
    int signal_led_base_pin{232};
    std::string spi_device{"/dev/spidev0.0"};
    std::string i2c_device{"/dev/i2c-1"};
    int frequency_detector_address{0x48};
};

struct NetworkConfig {
    std::string netconf_host{"0.0.0.0"};
    int netconf_port{830};
    std::string rest_host{"0.0.0.0"};
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
    bool enable_telegraf{true};
    std::string telegraf_socket{"/tmp/telegraf.sock"};
    int metrics_interval_seconds{30};
    bool enable_health_endpoint{true};
};

class ConfigManager {
public:
    explicit ConfigManager(const std::string& config_file_path);
    
    bool load_config();
    bool save_config() const;
    bool validate_config() const;
    
    const HardwareConfig& get_hardware_config() const { return hardware_config_; }
    const NetworkConfig& get_network_config() const { return network_config_; }
    const LoggingConfig& get_logging_config() const { return logging_config_; }
    const MonitoringConfig& get_monitoring_config() const { return monitoring_config_; }
    
    int get_web_port() const { return network_config_.rest_port; }
    std::string get_log_file() const { return logging_config_.log_file; }
    
    bool set_hardware_config(const HardwareConfig& config);
    bool set_network_config(const NetworkConfig& config);
    bool set_logging_config(const LoggingConfig& config);
    bool set_monitoring_config(const MonitoringConfig& config);
    
    nlohmann::json get_port_configs() const;
    bool set_port_configs(const nlohmann::json& configs);
    
    std::optional<std::string> get_yang_model_path() const;
    std::vector<std::string> get_yang_search_paths() const;

private:
    bool load_from_file();
    bool create_default_config();
    void merge_defaults();
    
    std::string config_file_path_;
    
    HardwareConfig hardware_config_;
    NetworkConfig network_config_;
    LoggingConfig logging_config_;
    MonitoringConfig monitoring_config_;
    
    nlohmann::json port_configs_;
    nlohmann::json raw_config_;
    
    bool config_loaded_{false};
    
    static constexpr const char* DEFAULT_YANG_MODEL = "/usr/share/splitter/yang/splitter.yang";
};

} // namespace splitter::core