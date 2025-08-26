#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace splitter::hardware {

struct FrequencyReading {
  double frequency_mhz{0.0};
  double signal_level_dbm{-100.0};
  std::chrono::system_clock::time_point timestamp;
  bool valid{false};
};

class FrequencyDetector {
public:
  FrequencyDetector();
  ~FrequencyDetector();

  bool initialize();
  void cleanup();

  double measure_frequency(int port_id);
  double get_frequency(int port_id) const;
  double get_signal_level(int port_id) const;

  FrequencyReading get_reading(int port_id) const;
  std::vector<FrequencyReading> get_all_readings() const;

  void start_continuous_measurement();
  void stop_continuous_measurement();

  bool is_healthy() const;
  std::string get_last_error() const;

  static constexpr int NUM_PORTS = 32;
  static constexpr double MIN_FREQUENCY_MHZ = 950.0;
  static constexpr double MAX_FREQUENCY_MHZ = 2150.0;

private:
  class I2CInterface;
  class SPIInterface;

  void measurement_thread();
  bool measure_port_frequency(int port_id);
  bool initialize_hardware();

  std::unique_ptr<I2CInterface> i2c_interface_;
  std::unique_ptr<SPIInterface> spi_interface_;

  std::vector<FrequencyReading> readings_;
  mutable std::mutex readings_mutex_;

  std::atomic<bool> initialized_{false};
  std::atomic<bool> continuous_measurement_{false};
  std::unique_ptr<std::thread> measurement_thread_;

  mutable std::string last_error_;
  std::atomic<bool> hardware_healthy_{true};

  std::chrono::steady_clock::time_point last_calibration_;
  static constexpr auto CALIBRATION_INTERVAL = std::chrono::minutes(10);
};

class FrequencyDetector::I2CInterface {
public:
  explicit I2CInterface(const std::string &device_path);
  ~I2CInterface();

  bool initialize();
  void cleanup();

  bool read_register(uint8_t reg, uint8_t &value);
  bool write_register(uint8_t reg, uint8_t value);
  bool read_block(uint8_t reg, uint8_t *data, size_t length);

  bool is_connected() const;

private:
  std::string device_path_;
  int fd_{-1};
  int device_address_{0x48};
  mutable std::mutex i2c_mutex_;
};

class FrequencyDetector::SPIInterface {
public:
  explicit SPIInterface(const std::string &device_path);
  ~SPIInterface();

  bool initialize();
  void cleanup();

  bool transfer(const uint8_t *tx_data, uint8_t *rx_data, size_t length);
  bool read_adc_channel(int channel, uint16_t &value);

  bool is_connected() const;

private:
  std::string device_path_;
  int fd_{-1};
  uint32_t spi_mode_{0};
  uint8_t spi_bits_per_word_{8};
  uint32_t spi_speed_hz_{1000000};
  mutable std::mutex spi_mutex_;
};

} // namespace splitter::hardware