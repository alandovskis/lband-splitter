#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <map>

namespace hardware {

// STM32 communication protocols
enum class STM32Protocol {
    UART,
    SPI,
    I2C,
    USB_CDC
};

// Command types for STM32 communication
enum class STM32Command {
    // Port control
    ENABLE_PORT = 0x01,
    DISABLE_PORT = 0x02,
    GET_PORT_STATUS = 0x03,
    
    // LED control
    SET_ENABLE_LED = 0x10,
    SET_SIGNAL_LED = 0x11,
    GET_LED_STATUS = 0x12,
    
    // Frequency detection
    GET_FREQUENCY = 0x20,
    GET_POWER_LEVEL = 0x21,
    SET_THRESHOLD = 0x22,
    START_MONITORING = 0x23,
    STOP_MONITORING = 0x24,
    
    // Display control
    UPDATE_DISPLAY = 0x30,
    SET_DISPLAY_BRIGHTNESS = 0x31,
    
    // System commands
    GET_SYSTEM_INFO = 0x40,
    RESET_SYSTEM = 0x41,
    RUN_SELF_TEST = 0x42,
    CALIBRATE_ADC = 0x43,
    
    // Configuration
    SET_CONFIG = 0x50,
    GET_CONFIG = 0x51,
    SAVE_CONFIG = 0x52,
    
    // Status and monitoring
    GET_ALL_STATUS = 0x60,
    GET_TEMPERATURE = 0x61,
    GET_VOLTAGE = 0x62
};

// Response status codes
enum class STM32Status {
    SUCCESS = 0x00,
    ERROR_INVALID_COMMAND = 0x01,
    ERROR_INVALID_PARAMETER = 0x02,
    ERROR_HARDWARE_FAULT = 0x03,
    ERROR_TIMEOUT = 0x04,
    ERROR_BUSY = 0x05,
    ERROR_NOT_INITIALIZED = 0x06
};

// STM32 communication packet structure
struct STM32Packet {
    uint8_t header;        // 0xAA
    uint8_t command;       // Command code
    uint8_t length;        // Data length
    std::vector<uint8_t> data;
    uint8_t checksum;      // XOR checksum
    
    STM32Packet() : header(0xAA), command(0), length(0), checksum(0) {}
};

// STM32 system information
struct STM32SystemInfo {
    std::string firmware_version;
    std::string hardware_version;
    uint32_t serial_number;
    uint16_t num_ports;
    uint16_t adc_resolution;
    float reference_voltage;
    uint32_t uptime_seconds;
};

// Port status from STM32
struct STM32PortStatus {
    uint8_t port_number;
    bool enabled;
    bool signal_detected;
    bool enable_led_state;
    bool signal_led_state;
    float frequency_mhz;
    float power_dbm;
    uint16_t raw_adc_value;
    uint8_t error_flags;
};

class STM32Interface {
public:
    STM32Interface(STM32Protocol protocol, const std::string& device_path);
    ~STM32Interface();

    bool initialize();
    void shutdown();
    
    bool is_connected() const { return connected_.load(); }
    
    // Port control commands
    bool enable_port(uint8_t port_number);
    bool disable_port(uint8_t port_number);
    STM32PortStatus get_port_status(uint8_t port_number);
    std::vector<STM32PortStatus> get_all_port_status();
    
    // LED control
    bool set_enable_led(uint8_t port_number, bool state);
    bool set_signal_led(uint8_t port_number, bool state);
    
    // Frequency detection
    float get_frequency(uint8_t port_number);
    float get_power_level(uint8_t port_number);
    bool set_signal_threshold(uint8_t port_number, float threshold_dbm);
    bool start_frequency_monitoring(uint8_t port_number);
    bool stop_frequency_monitoring(uint8_t port_number);
    
    // Display control
    bool update_display(uint8_t port_number, const std::string& text);
    bool set_display_brightness(uint8_t port_number, uint8_t brightness);
    
    // System commands
    STM32SystemInfo get_system_info();
    bool reset_system();
    bool run_self_test();
    bool calibrate_adc();
    
    // Configuration management
    bool set_configuration(const std::map<std::string, std::string>& config);
    std::map<std::string, std::string> get_configuration();
    bool save_configuration();
    
    // System monitoring
    float get_system_temperature();
    float get_system_voltage();
    
    // Communication settings
    void set_timeout(uint32_t timeout_ms) { timeout_ms_ = timeout_ms; }
    void set_retries(uint8_t retries) { max_retries_ = retries; }
    
    // Status and error handling
    STM32Status get_last_error() const { return last_error_; }
    std::string get_error_string() const;
    uint32_t get_communication_errors() const { return comm_errors_; }
    void clear_error_counters();

private:
    STM32Protocol protocol_;
    std::string device_path_;
    int device_fd_;
    
    std::atomic<bool> connected_;
    std::atomic<bool> monitoring_;
    
    uint32_t timeout_ms_;
    uint8_t max_retries_;
    STM32Status last_error_;
    std::atomic<uint32_t> comm_errors_;
    
    mutable std::mutex comm_mutex_;
    std::unique_ptr<std::thread> monitor_thread_;
    
    // Low-level communication
    bool open_device();
    void close_device();
    bool send_packet(const STM32Packet& packet);
    bool receive_packet(STM32Packet& packet);
    
    // Protocol-specific implementations
    bool uart_send(const std::vector<uint8_t>& data);
    bool uart_receive(std::vector<uint8_t>& data, size_t expected_length);
    bool spi_send(const std::vector<uint8_t>& data);
    bool spi_receive(std::vector<uint8_t>& data, size_t expected_length);
    bool i2c_send(const std::vector<uint8_t>& data);
    bool i2c_receive(std::vector<uint8_t>& data, size_t expected_length);
    bool usb_cdc_send(const std::vector<uint8_t>& data);
    bool usb_cdc_receive(std::vector<uint8_t>& data, size_t expected_length);
    
    // Packet handling
    STM32Packet create_command_packet(STM32Command cmd, const std::vector<uint8_t>& data = {});
    bool send_command(STM32Command cmd, const std::vector<uint8_t>& data = {});
    bool send_command_with_response(STM32Command cmd, 
                                    const std::vector<uint8_t>& data,
                                    STM32Packet& response);
    
    uint8_t calculate_checksum(const STM32Packet& packet);
    bool verify_checksum(const STM32Packet& packet);
    
    // Data conversion helpers
    std::vector<uint8_t> float_to_bytes(float value);
    float bytes_to_float(const std::vector<uint8_t>& bytes, size_t offset = 0);
    std::vector<uint8_t> uint16_to_bytes(uint16_t value);
    uint16_t bytes_to_uint16(const std::vector<uint8_t>& bytes, size_t offset = 0);
    std::vector<uint8_t> uint32_to_bytes(uint32_t value);
    uint32_t bytes_to_uint32(const std::vector<uint8_t>& bytes, size_t offset = 0);
    
    // Error handling
    void set_error(STM32Status error);
    void increment_comm_error();
    
    // Monitoring thread
    void monitoring_thread();
};

// Factory function to create STM32 interface
std::unique_ptr<STM32Interface> create_stm32_interface(
    STM32Protocol protocol, 
    const std::string& device_path
);

} // namespace hardware