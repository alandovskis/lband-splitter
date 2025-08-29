#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace splitter::hardware {

struct STM32F4Reading {
  double frequency_mhz{0.0};
  double snr_db{0.0};
  double signal_level_dbm{-100.0};
  uint16_t raw_frequency{0};
  uint16_t raw_snr{0};
  std::chrono::system_clock::time_point timestamp;
  bool valid{false};
  uint8_t port_id{0};
};



// Communication protocol definitions
namespace protocol {
constexpr uint8_t STM32_CMD_READ_FREQUENCY = 0x01;
constexpr uint8_t STM32_CMD_READ_SNR = 0x02;
constexpr uint8_t STM32_CMD_ENABLE_PORT = 0x03;
constexpr uint8_t STM32_CMD_GET_STATUS = 0x05;
constexpr uint8_t STM32_CMD_CALIBRATE = 0x06;
constexpr uint8_t STM32_CMD_RESET = 0x07;
constexpr uint8_t STM32_CMD_SIGNAL_DETECTION = 0x08;

constexpr uint8_t STM32_RESP_OK = 0x00;
constexpr uint8_t STM32_RESP_ERROR = 0xFF;
constexpr uint8_t STM32_RESP_BUSY = 0xFE;
constexpr uint8_t STM32_RESP_INVALID_CMD = 0xFD;

constexpr size_t STM32_MAX_PACKET_SIZE = 64;
constexpr uint32_t STM32_TIMEOUT_MS = 100;
} // namespace protocol

class STM32F4Controller {
public:
  explicit STM32F4Controller(int port_id, const std::string &uart_device);
  ~STM32F4Controller();

  bool initialize();
  void cleanup();

  // Frequency and SNR measurement
  bool read_frequency_and_snr(STM32F4Reading &reading);
  STM32F4Reading get_last_reading() const;
  bool start_continuous_measurement();
  bool stop_continuous_measurement();
  
  // Port state control for autonomous LED behavior
  bool enable_port(bool enabled);
  bool set_signal_detection(bool detected);



  // Calibration and maintenance
  bool calibrate_frequency_detector();
  bool reset_mcu();
  bool get_firmware_version(std::string &version);

  // Status and diagnostics
  bool is_healthy() const;
  bool is_connected() const;
  std::string get_last_error() const;
  uint8_t get_port_id() const { return port_id_; }

  // Static constants
  static constexpr int NUM_PORTS = 32;
  static constexpr double MIN_FREQUENCY_MHZ = 950.0;
  static constexpr double MAX_FREQUENCY_MHZ = 2150.0;
  static constexpr double MIN_SNR_DB = -30.0;
  static constexpr double MAX_SNR_DB = 40.0;

private:
  class UARTInterface;

  struct CommandPacket {
    uint8_t command;
    uint8_t port_id;
    uint8_t length;
    uint8_t data[protocol::STM32_MAX_PACKET_SIZE - 3];
    uint8_t checksum;
  };

  struct ResponsePacket {
    uint8_t status;
    uint8_t length;
    uint8_t data[protocol::STM32_MAX_PACKET_SIZE - 2];
  };

  bool send_command(uint8_t cmd, const uint8_t *data = nullptr,
                    size_t data_len = 0);
  bool receive_response(ResponsePacket &response);
  bool send_command_with_response(uint8_t cmd, ResponsePacket &response,
                                  const uint8_t *data = nullptr,
                                  size_t data_len = 0);

  void measurement_thread();
  uint8_t calculate_checksum(const CommandPacket &packet) const;
  bool verify_response_checksum(const ResponsePacket &response) const;

  std::unique_ptr<UARTInterface> uart_interface_;
  uint8_t port_id_;
  std::string uart_device_;

  STM32F4Reading last_reading_;
  mutable std::mutex reading_mutex_;

  std::atomic<bool> initialized_{false};
  std::atomic<bool> continuous_measurement_{false};
  std::atomic<bool> hardware_healthy_{true};
  std::unique_ptr<std::thread> measurement_thread_;

  mutable std::string last_error_;
  std::chrono::steady_clock::time_point last_communication_;

  // Communication statistics
  std::atomic<uint32_t> successful_commands_{0};
  std::atomic<uint32_t> failed_commands_{0};
  std::atomic<uint32_t> timeout_count_{0};
};

class STM32F4Controller::UARTInterface {
public:
  explicit UARTInterface(const std::string &device_path);
  ~UARTInterface();

  bool initialize(uint32_t baud_rate = 115200);
  void cleanup();

  bool send_data(const uint8_t *data, size_t length);
  bool receive_data(uint8_t *data, size_t max_length, size_t &received_length,
                    uint32_t timeout_ms = protocol::STM32_TIMEOUT_MS);

  bool is_connected() const;
  void flush_buffers();

private:
  bool configure_port(uint32_t baud_rate);

  std::string device_path_;
  int fd_{-1};
  mutable std::mutex uart_mutex_;
};

// Manager class for all STM32F4 controllers (one per port)
class STM32F4Manager {
public:
  STM32F4Manager();
  ~STM32F4Manager();

  bool initialize();
  void cleanup();

  bool configure_port(uint8_t port_id, const std::string &uart_device);
  STM32F4Controller *get_controller(uint8_t port_id);

  // Bulk operations
  std::vector<STM32F4Reading> read_all_frequencies();

  // Status monitoring
  std::vector<uint8_t> get_healthy_ports() const;
  std::vector<uint8_t> get_unhealthy_ports() const;
  bool are_all_ports_healthy() const;

  // Calibration
  bool calibrate_all_ports();
  bool reset_all_mcus();

private:
  std::vector<std::unique_ptr<STM32F4Controller>> controllers_;
  mutable std::mutex controllers_mutex_;

  std::atomic<bool> initialized_{false};

  void health_monitor_thread();
  std::unique_ptr<std::thread> health_monitor_thread_;
  std::atomic<bool> health_monitoring_{false};
};

} // namespace splitter::hardware