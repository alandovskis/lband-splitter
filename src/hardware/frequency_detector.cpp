#include "frequency_detector.h"
#include "../utils/logger.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
// #include <linux/i2c-dev.h>   // Not available on macOS
// #include <linux/spi/spidev.h> // Not available on macOS
#include <algorithm>
#include <cmath>

// Mock Linux constants for macOS build
#ifndef I2C_SLAVE
#define I2C_SLAVE 0x0703
#endif

#ifndef SPI_IOC_MESSAGE
#define SPI_IOC_MESSAGE(n) 0
#define SPI_IOC_WR_MODE 0x40016B01
#define SPI_IOC_WR_BITS_PER_WORD 0x40016B03
#define SPI_IOC_WR_MAX_SPEED_HZ 0x40046B04
struct spi_ioc_transfer {
  unsigned long tx_buf;
  unsigned long rx_buf;
  uint32_t len;
  uint32_t delay_usecs;
  uint32_t speed_hz;
  uint8_t bits_per_word;
};
#endif

namespace splitter::hardware {

FrequencyDetector::FrequencyDetector()
    : readings_(NUM_PORTS),
      last_calibration_(std::chrono::steady_clock::now()) {}

FrequencyDetector::~FrequencyDetector() { cleanup(); }

bool FrequencyDetector::initialize() {
  if (initialized_) {
    return true;
  }

  try {
    i2c_interface_ = std::make_unique<I2CInterface>("/dev/i2c-1");
    if (!i2c_interface_->initialize()) {
      last_error_ = "Failed to initialize I2C interface";
      return false;
    }

    spi_interface_ = std::make_unique<SPIInterface>("/dev/spidev0.0");
    if (!spi_interface_->initialize()) {
      last_error_ = "Failed to initialize SPI interface";
      return false;
    }

    if (!initialize_hardware()) {
      last_error_ = "Failed to initialize frequency detection hardware";
      return false;
    }

    std::lock_guard<std::mutex> lock(readings_mutex_);
    for (auto &reading : readings_) {
      reading.timestamp = std::chrono::system_clock::now();
      reading.valid = false;
    }

    initialized_ = true;
    hardware_healthy_ = true;

    utils::Logger::info("Frequency detector initialized successfully");
    return true;

  } catch (const std::exception &e) {
    last_error_ = "Exception during initialization: " + std::string(e.what());
    utils::Logger::error("FrequencyDetector initialization failed: {}",
                         e.what());
    return false;
  }
}

void FrequencyDetector::cleanup() {
  if (!initialized_) {
    return;
  }

  stop_continuous_measurement();

  spi_interface_.reset();
  i2c_interface_.reset();

  initialized_ = false;
  utils::Logger::info("Frequency detector cleanup complete");
}

double FrequencyDetector::measure_frequency(int port_id) {
  if (!initialized_ || port_id < 0 || port_id >= NUM_PORTS) {
    return 0.0;
  }

  if (!measure_port_frequency(port_id)) {
    return 0.0;
  }

  std::lock_guard<std::mutex> lock(readings_mutex_);
  return readings_[port_id].frequency_mhz;
}

double FrequencyDetector::get_frequency(int port_id) const {
  if (port_id < 0 || port_id >= NUM_PORTS) {
    return 0.0;
  }

  std::lock_guard<std::mutex> lock(readings_mutex_);
  return readings_[port_id].valid ? readings_[port_id].frequency_mhz : 0.0;
}

double FrequencyDetector::get_signal_level(int port_id) const {
  if (port_id < 0 || port_id >= NUM_PORTS) {
    return -100.0;
  }

  std::lock_guard<std::mutex> lock(readings_mutex_);
  return readings_[port_id].valid ? readings_[port_id].signal_level_dbm
                                  : -100.0;
}

FrequencyReading FrequencyDetector::get_reading(int port_id) const {
  if (port_id < 0 || port_id >= NUM_PORTS) {
    return {};
  }

  std::lock_guard<std::mutex> lock(readings_mutex_);
  return readings_[port_id];
}

std::vector<FrequencyReading> FrequencyDetector::get_all_readings() const {
  std::lock_guard<std::mutex> lock(readings_mutex_);
  return readings_;
}

void FrequencyDetector::start_continuous_measurement() {
  if (!initialized_ || continuous_measurement_) {
    return;
  }

  continuous_measurement_ = true;
  measurement_thread_ = std::make_unique<std::thread>(
      &FrequencyDetector::measurement_thread, this);

  utils::Logger::info("Started continuous frequency measurement");
}

void FrequencyDetector::stop_continuous_measurement() {
  if (!continuous_measurement_) {
    return;
  }

  continuous_measurement_ = false;

  if (measurement_thread_ && measurement_thread_->joinable()) {
    measurement_thread_->join();
  }

  measurement_thread_.reset();
  utils::Logger::info("Stopped continuous frequency measurement");
}

bool FrequencyDetector::is_healthy() const {
  return initialized_ && hardware_healthy_ && i2c_interface_ &&
         i2c_interface_->is_connected() && spi_interface_ &&
         spi_interface_->is_connected();
}

std::string FrequencyDetector::get_last_error() const { return last_error_; }

void FrequencyDetector::measurement_thread() {
  utils::Logger::debug("Frequency measurement thread started");

  while (continuous_measurement_) {
    for (int port_id = 0; port_id < NUM_PORTS && continuous_measurement_;
         ++port_id) {
      measure_port_frequency(port_id);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    auto now = std::chrono::steady_clock::now();
    if (now - last_calibration_ > CALIBRATION_INTERVAL) {
      initialize_hardware();
      last_calibration_ = now;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  utils::Logger::debug("Frequency measurement thread stopped");
}

bool FrequencyDetector::measure_port_frequency(int port_id) {
  try {
    uint16_t adc_value;
    if (!spi_interface_->read_adc_channel(port_id % 8, adc_value)) {
      hardware_healthy_ = false;
      return false;
    }

    double voltage = (adc_value / 4095.0) * 3.3;

    double frequency_mhz =
        MIN_FREQUENCY_MHZ +
        (voltage / 3.3) * (MAX_FREQUENCY_MHZ - MIN_FREQUENCY_MHZ);

    double signal_level_dbm = -100.0 + (voltage * 60.0);

    std::lock_guard<std::mutex> lock(readings_mutex_);
    readings_[port_id].frequency_mhz = frequency_mhz;
    readings_[port_id].signal_level_dbm = signal_level_dbm;
    readings_[port_id].timestamp = std::chrono::system_clock::now();
    readings_[port_id].valid = (voltage > 0.1);

    hardware_healthy_ = true;
    return true;

  } catch (const std::exception &e) {
    last_error_ = "Exception measuring frequency for port " +
                  std::to_string(port_id) + ": " + e.what();
    hardware_healthy_ = false;
    return false;
  }
}

bool FrequencyDetector::initialize_hardware() {
  if (!i2c_interface_ || !spi_interface_) {
    return false;
  }

  uint8_t config_reg = 0x80;
  if (!i2c_interface_->write_register(0x00, config_reg)) {
    last_error_ = "Failed to configure frequency detector";
    return false;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  return true;
}

FrequencyDetector::I2CInterface::I2CInterface(const std::string &device_path)
    : device_path_(device_path) {}

FrequencyDetector::I2CInterface::~I2CInterface() { cleanup(); }

bool FrequencyDetector::I2CInterface::initialize() {
  std::lock_guard<std::mutex> lock(i2c_mutex_);

  fd_ = open(device_path_.c_str(), O_RDWR);
  if (fd_ < 0) {
    utils::Logger::error("Failed to open I2C device: {}", device_path_);
    return false;
  }

  if (ioctl(fd_, I2C_SLAVE, device_address_) < 0) {
    utils::Logger::error("Failed to set I2C slave address: 0x{:02x}",
                         device_address_);
    close(fd_);
    fd_ = -1;
    return false;
  }

  utils::Logger::debug("I2C interface initialized on {}", device_path_);
  return true;
}

void FrequencyDetector::I2CInterface::cleanup() {
  std::lock_guard<std::mutex> lock(i2c_mutex_);

  if (fd_ >= 0) {
    close(fd_);
    fd_ = -1;
  }
}

bool FrequencyDetector::I2CInterface::read_register(uint8_t reg,
                                                    uint8_t &value) {
  std::lock_guard<std::mutex> lock(i2c_mutex_);

  if (fd_ < 0) {
    return false;
  }

  if (write(fd_, &reg, 1) != 1) {
    return false;
  }

  if (read(fd_, &value, 1) != 1) {
    return false;
  }

  return true;
}

bool FrequencyDetector::I2CInterface::write_register(uint8_t reg,
                                                     uint8_t value) {
  std::lock_guard<std::mutex> lock(i2c_mutex_);

  if (fd_ < 0) {
    return false;
  }

  uint8_t buffer[2] = {reg, value};
  return write(fd_, buffer, 2) == 2;
}

bool FrequencyDetector::I2CInterface::read_block(uint8_t reg, uint8_t *data,
                                                 size_t length) {
  std::lock_guard<std::mutex> lock(i2c_mutex_);

  if (fd_ < 0 || !data) {
    return false;
  }

  if (write(fd_, &reg, 1) != 1) {
    return false;
  }

  return read(fd_, data, length) == static_cast<ssize_t>(length);
}

bool FrequencyDetector::I2CInterface::is_connected() const {
  std::lock_guard<std::mutex> lock(i2c_mutex_);
  return fd_ >= 0;
}

FrequencyDetector::SPIInterface::SPIInterface(const std::string &device_path)
    : device_path_(device_path) {}

FrequencyDetector::SPIInterface::~SPIInterface() { cleanup(); }

bool FrequencyDetector::SPIInterface::initialize() {
  std::lock_guard<std::mutex> lock(spi_mutex_);

  fd_ = open(device_path_.c_str(), O_RDWR);
  if (fd_ < 0) {
    utils::Logger::error("Failed to open SPI device: {}", device_path_);
    return false;
  }

  if (ioctl(fd_, SPI_IOC_WR_MODE, &spi_mode_) < 0 ||
      ioctl(fd_, SPI_IOC_WR_BITS_PER_WORD, &spi_bits_per_word_) < 0 ||
      ioctl(fd_, SPI_IOC_WR_MAX_SPEED_HZ, &spi_speed_hz_) < 0) {

    utils::Logger::error("Failed to configure SPI device");
    close(fd_);
    fd_ = -1;
    return false;
  }

  utils::Logger::debug("SPI interface initialized on {}", device_path_);
  return true;
}

void FrequencyDetector::SPIInterface::cleanup() {
  std::lock_guard<std::mutex> lock(spi_mutex_);

  if (fd_ >= 0) {
    close(fd_);
    fd_ = -1;
  }
}

bool FrequencyDetector::SPIInterface::transfer(const uint8_t *tx_data,
                                               uint8_t *rx_data,
                                               size_t length) {
  std::lock_guard<std::mutex> lock(spi_mutex_);

  if (fd_ < 0) {
    return false;
  }

  struct spi_ioc_transfer tr = {};
  tr.tx_buf = (unsigned long)tx_data;
  tr.rx_buf = (unsigned long)rx_data;
  tr.len = length;
  tr.delay_usecs = 0;
  tr.speed_hz = spi_speed_hz_;
  tr.bits_per_word = spi_bits_per_word_;

  return ioctl(fd_, SPI_IOC_MESSAGE(1), &tr) >= 0;
}

bool FrequencyDetector::SPIInterface::read_adc_channel(int channel,
                                                       uint16_t &value) {
  if (channel < 0 || channel > 7) {
    return false;
  }

  uint8_t tx_data[3] = {static_cast<uint8_t>(0x06 | ((channel & 0x04) >> 2)),
                        static_cast<uint8_t>((channel & 0x03) << 6), 0x00};

  uint8_t rx_data[3] = {0};

  if (!transfer(tx_data, rx_data, 3)) {
    return false;
  }

  value = ((rx_data[1] & 0x0F) << 8) | rx_data[2];
  return true;
}

bool FrequencyDetector::SPIInterface::is_connected() const {
  std::lock_guard<std::mutex> lock(spi_mutex_);
  return fd_ >= 0;
}

} // namespace splitter::hardware