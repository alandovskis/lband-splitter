#include "stm32f4_controller.h"
#include "../utils/logger.h"

#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <sstream>
#include <termios.h>
#include <unistd.h>

namespace splitter::hardware {

STM32F4Controller::STM32F4Controller(int port_id,
                                     const std::string &uart_device)
    : port_id_(static_cast<uint8_t>(port_id)), uart_device_(uart_device) {}

STM32F4Controller::~STM32F4Controller() { cleanup(); }

bool STM32F4Controller::initialize() {
  if (initialized_) {
    return true;
  }

  try {
    uart_interface_ = std::make_unique<UARTInterface>(uart_device_);
    if (!uart_interface_->initialize()) {
      last_error_ = "Failed to initialize UART interface";
      return false;
    }

    ResponsePacket response;
    if (!send_command_with_response(protocol::STM32_CMD_GET_STATUS, response)) {
      last_error_ = "Failed to communicate with STM32F4 controller";
      return false;
    }

    initialized_ = true;
    hardware_healthy_ = true;
    last_communication_ = std::chrono::steady_clock::now();

    utils::Logger::info("STM32F4Controller port {} initialized successfully",
                        port_id_);
    return true;

  } catch (const std::exception &e) {
    last_error_ = "Exception during initialization: " + std::string(e.what());
    utils::Logger::error("STM32F4Controller port {} initialization failed: {}",
                         port_id_, e.what());
    return false;
  }
}

void STM32F4Controller::cleanup() {
  if (!initialized_) {
    return;
  }

  stop_continuous_measurement();
  uart_interface_.reset();
  initialized_ = false;

  utils::Logger::info("STM32F4Controller port {} cleanup complete", port_id_);
}

bool STM32F4Controller::read_frequency_and_snr(STM32F4Reading &reading) {
  if (!initialized_) {
    return false;
  }

  ResponsePacket freq_response, snr_response;

  if (!send_command_with_response(protocol::STM32_CMD_READ_FREQUENCY,
                                  freq_response) ||
      !send_command_with_response(protocol::STM32_CMD_READ_SNR, snr_response)) {
    hardware_healthy_ = false;
    return false;
  }

  if (freq_response.length >= 4 && snr_response.length >= 4) {
    uint16_t raw_freq = (freq_response.data[0] << 8) | freq_response.data[1];
    uint16_t raw_snr = (snr_response.data[0] << 8) | snr_response.data[1];

    reading.raw_frequency = raw_freq;
    reading.raw_snr = raw_snr;

    reading.frequency_mhz =
        MIN_FREQUENCY_MHZ + (static_cast<double>(raw_freq) / 65535.0) *
                                (MAX_FREQUENCY_MHZ - MIN_FREQUENCY_MHZ);

    reading.snr_db = MIN_SNR_DB + (static_cast<double>(raw_snr) / 65535.0) *
                                      (MAX_SNR_DB - MIN_SNR_DB);

    reading.signal_level_dbm =
        -100.0 + (static_cast<double>(raw_freq) / 65535.0) * 60.0;

    reading.timestamp = std::chrono::system_clock::now();
    reading.valid = (raw_freq > 100);
    reading.port_id = port_id_;

    {
      std::lock_guard<std::mutex> lock(reading_mutex_);
      last_reading_ = reading;
    }

    hardware_healthy_ = true;
    last_communication_ = std::chrono::steady_clock::now();
    successful_commands_++;
    return true;
  }

  failed_commands_++;
  return false;
}

STM32F4Reading STM32F4Controller::get_last_reading() const {
  std::lock_guard<std::mutex> lock(reading_mutex_);
  return last_reading_;
}

bool STM32F4Controller::start_continuous_measurement() {
  if (!initialized_ || continuous_measurement_) {
    return false;
  }

  continuous_measurement_ = true;
  measurement_thread_ = std::make_unique<std::thread>(
      &STM32F4Controller::measurement_thread, this);

  utils::Logger::info("Started continuous measurement for STM32F4 port {}",
                      port_id_);
  return true;
}

bool STM32F4Controller::stop_continuous_measurement() {
  if (!continuous_measurement_) {
    return true;
  }

  continuous_measurement_ = false;

  if (measurement_thread_ && measurement_thread_->joinable()) {
    measurement_thread_->join();
  }
  measurement_thread_.reset();

  utils::Logger::info("Stopped continuous measurement for STM32F4 port {}",
                      port_id_);
  return true;
}

bool STM32F4Controller::enable_port(bool enabled) {
  if (!initialized_) {
    return false;
  }
  
  uint8_t enable_data = enabled ? 1 : 0;
  ResponsePacket response;
  return send_command_with_response(protocol::STM32_CMD_ENABLE_PORT, response,
                                    &enable_data, 1);
}

bool STM32F4Controller::set_signal_detection(bool detected) {
  if (!initialized_) {
    return false;
  }
  
  uint8_t detection_data = detected ? 1 : 0;
  ResponsePacket response;
  return send_command_with_response(protocol::STM32_CMD_SIGNAL_DETECTION, response,
                                    &detection_data, 1);
}

bool STM32F4Controller::update_display(const STM32F4DisplayData &data) {
  if (!initialized_) {
    return false;
  }

  std::stringstream freq_str;
  freq_str << std::fixed << std::setprecision(1) << data.frequency_mhz;
  std::string freq_text = freq_str.str();

  std::stringstream snr_str;
  snr_str << std::fixed << std::setprecision(1) << data.snr_db;
  std::string snr_text = snr_str.str();

  std::string display_text = data.custom_text.empty()
                                 ? freq_text + "MHz " + snr_text + "dB"
                                 : data.custom_text;

  if (display_text.length() > 32) {
    display_text = display_text.substr(0, 32);
  }

  uint8_t display_data[36];
  display_data[0] = data.brightness;
  display_data[1] = data.signal_present ? 1u : 0u;
  display_data[2] = static_cast<uint8_t>(display_text.length());
  std::memcpy(&display_data[3], display_text.c_str(), display_text.length());

  ResponsePacket response;
  return send_command_with_response(protocol::STM32_CMD_UPDATE_DISPLAY,
                                    response, display_data,
                                    3 + display_text.length());
}

bool STM32F4Controller::set_display_brightness(uint8_t brightness) {
  STM32F4DisplayData data;
  data.brightness = brightness;
  return update_display(data);
}

bool STM32F4Controller::clear_display() {
  STM32F4DisplayData data;
  data.custom_text = "";
  return update_display(data);
}

bool STM32F4Controller::calibrate_frequency_detector() {
  if (!initialized_) {
    return false;
  }

  ResponsePacket response;
  return send_command_with_response(protocol::STM32_CMD_CALIBRATE, response);
}

bool STM32F4Controller::reset_mcu() {
  if (!initialized_) {
    return false;
  }

  ResponsePacket response;
  if (send_command_with_response(protocol::STM32_CMD_RESET, response)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    return initialize();
  }
  return false;
}

bool STM32F4Controller::get_firmware_version(std::string &version) {
  if (!initialized_) {
    return false;
  }

  ResponsePacket response;
  if (send_command_with_response(protocol::STM32_CMD_GET_STATUS, response) &&
      response.length >= 4) {

    uint8_t major = response.data[0];
    uint8_t minor = response.data[1];
    uint8_t patch = response.data[2];

    std::stringstream ss;
    ss << static_cast<int>(major) << "." << static_cast<int>(minor) << "."
       << static_cast<int>(patch);
    version = ss.str();
    return true;
  }
  return false;
}

bool STM32F4Controller::is_healthy() const {
  auto now = std::chrono::steady_clock::now();
  auto time_since_comm = std::chrono::duration_cast<std::chrono::seconds>(
                             now - last_communication_)
                             .count();

  return initialized_ && hardware_healthy_ && uart_interface_ &&
         uart_interface_->is_connected() && time_since_comm < 30;
}

bool STM32F4Controller::is_connected() const {
  return initialized_ && uart_interface_ && uart_interface_->is_connected();
}

std::string STM32F4Controller::get_last_error() const { return last_error_; }

bool STM32F4Controller::send_command(uint8_t cmd, const uint8_t *data,
                                     size_t data_len) {
  if (!uart_interface_ || data_len > protocol::STM32_MAX_PACKET_SIZE - 3) {
    return false;
  }

  CommandPacket packet;
  packet.command = cmd;
  packet.port_id = port_id_;
  packet.length = static_cast<uint8_t>(data_len);

  if (data && data_len > 0) {
    std::memcpy(packet.data, data, data_len);
  }

  packet.checksum = calculate_checksum(packet);

  size_t packet_size = 3 + data_len + 1;
  return uart_interface_->send_data(reinterpret_cast<const uint8_t *>(&packet),
                                    packet_size);
}

bool STM32F4Controller::receive_response(ResponsePacket &response) {
  if (!uart_interface_) {
    return false;
  }

  size_t received_length;
  if (!uart_interface_->receive_data(reinterpret_cast<uint8_t *>(&response),
                                     sizeof(response), received_length,
                                     protocol::STM32_TIMEOUT_MS)) {
    timeout_count_++;
    return false;
  }

  return received_length >= 2 && verify_response_checksum(response);
}

bool STM32F4Controller::send_command_with_response(uint8_t cmd,
                                                   ResponsePacket &response,
                                                   const uint8_t *data,
                                                   size_t data_len) {
  if (!send_command(cmd, data, data_len)) {
    return false;
  }

  if (!receive_response(response)) {
    return false;
  }

  if (response.status != protocol::STM32_RESP_OK) {
    std::stringstream ss;
    ss << "Command 0x" << std::hex << static_cast<int>(cmd)
       << " failed with status 0x" << std::hex
       << static_cast<int>(response.status);
    last_error_ = ss.str();
    return false;
  }

  return true;
}

void STM32F4Controller::measurement_thread() {
  utils::Logger::debug("STM32F4 measurement thread started for port {}",
                       port_id_);

  while (continuous_measurement_) {
    STM32F4Reading reading;
    if (read_frequency_and_snr(reading)) {
      utils::Logger::trace("Port {} - Freq: {:.1f} MHz, SNR: {:.1f} dB",
                           port_id_, reading.frequency_mhz, reading.snr_db);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  utils::Logger::debug("STM32F4 measurement thread stopped for port {}",
                       port_id_);
}

uint8_t
STM32F4Controller::calculate_checksum(const CommandPacket &packet) const {
  uint8_t checksum = 0;
  checksum ^= packet.command;
  checksum ^= packet.port_id;
  checksum ^= packet.length;

  for (size_t i = 0; i < packet.length; ++i) {
    checksum ^= packet.data[i];
  }

  return checksum;
}

bool STM32F4Controller::verify_response_checksum(
    const ResponsePacket &response) const {
  if (response.length == 0) {
    return true;
  }

  uint8_t calculated_checksum = response.status ^ response.length;
  for (size_t i = 0; i < response.length - 1; ++i) {
    calculated_checksum ^= response.data[i];
  }

  return calculated_checksum == response.data[response.length - 1];
}

STM32F4Controller::UARTInterface::UARTInterface(const std::string &device_path)
    : device_path_(device_path) {}

STM32F4Controller::UARTInterface::~UARTInterface() { cleanup(); }

bool STM32F4Controller::UARTInterface::initialize(uint32_t baud_rate) {
  std::lock_guard<std::mutex> lock(uart_mutex_);

  fd_ = open(device_path_.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd_ < 0) {
    utils::Logger::error("Failed to open UART device: {}", device_path_);
    return false;
  }

  if (!configure_port(baud_rate)) {
    close(fd_);
    fd_ = -1;
    return false;
  }

  utils::Logger::debug("UART interface initialized on {}", device_path_);
  return true;
}

void STM32F4Controller::UARTInterface::cleanup() {
  std::lock_guard<std::mutex> lock(uart_mutex_);

  if (fd_ >= 0) {
    close(fd_);
    fd_ = -1;
  }
}

bool STM32F4Controller::UARTInterface::configure_port(uint32_t baud_rate) {
  struct termios tty;
  if (tcgetattr(fd_, &tty) != 0) {
    return false;
  }

  speed_t speed;
  switch (baud_rate) {
  case 9600:
    speed = B9600;
    break;
  case 19200:
    speed = B19200;
    break;
  case 38400:
    speed = B38400;
    break;
  case 57600:
    speed = B57600;
    break;
  case 115200:
    speed = B115200;
    break;
  default:
    speed = B115200;
    break;
  }

  cfsetospeed(&tty, speed);
  cfsetispeed(&tty, speed);

  tty.c_cflag &= ~PARENB;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;
  tty.c_cflag &= ~CRTSCTS;
  tty.c_cflag |= CREAD | CLOCAL;

  tty.c_lflag &= ~ICANON;
  tty.c_lflag &= ~ECHO;
  tty.c_lflag &= ~ECHOE;
  tty.c_lflag &= ~ECHONL;
  tty.c_lflag &= ~ISIG;

  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);

  tty.c_oflag &= ~OPOST;
  tty.c_oflag &= ~ONLCR;

  tty.c_cc[VTIME] = 10;
  tty.c_cc[VMIN] = 0;

  return tcsetattr(fd_, TCSANOW, &tty) == 0;
}

bool STM32F4Controller::UARTInterface::send_data(const uint8_t *data,
                                                 size_t length) {
  std::lock_guard<std::mutex> lock(uart_mutex_);

  if (fd_ < 0 || !data) {
    return false;
  }

  ssize_t written = write(fd_, data, length);
  return written == static_cast<ssize_t>(length);
}

bool STM32F4Controller::UARTInterface::receive_data(uint8_t *data,
                                                    size_t max_length,
                                                    size_t &received_length,
                                                    uint32_t timeout_ms) {
  std::lock_guard<std::mutex> lock(uart_mutex_);

  if (fd_ < 0 || !data) {
    return false;
  }

  fd_set read_fds;
  struct timeval timeout;

  FD_ZERO(&read_fds);
  FD_SET(fd_, &read_fds);

  timeout.tv_sec = timeout_ms / 1000;
  timeout.tv_usec = (timeout_ms % 1000) * 1000;

  int result = select(fd_ + 1, &read_fds, nullptr, nullptr, &timeout);
  if (result <= 0) {
    return false;
  }

  ssize_t bytes_read = read(fd_, data, max_length);
  if (bytes_read > 0) {
    received_length = static_cast<size_t>(bytes_read);
    return true;
  }

  return false;
}

bool STM32F4Controller::UARTInterface::is_connected() const {
  std::lock_guard<std::mutex> lock(uart_mutex_);
  return fd_ >= 0;
}

void STM32F4Controller::UARTInterface::flush_buffers() {
  std::lock_guard<std::mutex> lock(uart_mutex_);
  if (fd_ >= 0) {
    tcflush(fd_, TCIOFLUSH);
  }
}

STM32F4Manager::STM32F4Manager() {
  controllers_.resize(STM32F4Controller::NUM_PORTS);
}

STM32F4Manager::~STM32F4Manager() { cleanup(); }

bool STM32F4Manager::initialize() {
  if (initialized_) {
    return true;
  }

  for (int port_id = 0; port_id < STM32F4Controller::NUM_PORTS; ++port_id) {
    std::string uart_device = "/dev/ttyS" + std::to_string(port_id);
    if (!configure_port(static_cast<uint8_t>(port_id), uart_device)) {
      utils::Logger::warning(
          "Failed to configure STM32F4 controller for port {}", port_id);
    }
  }

  health_monitoring_ = true;
  health_monitor_thread_ = std::make_unique<std::thread>(
      &STM32F4Manager::health_monitor_thread, this);

  initialized_ = true;
  utils::Logger::info("STM32F4Manager initialized successfully");
  return true;
}

void STM32F4Manager::cleanup() {
  if (!initialized_) {
    return;
  }

  health_monitoring_ = false;
  if (health_monitor_thread_ && health_monitor_thread_->joinable()) {
    health_monitor_thread_->join();
  }
  health_monitor_thread_.reset();

  std::lock_guard<std::mutex> lock(controllers_mutex_);
  for (auto &controller : controllers_) {
    if (controller) {
      controller->cleanup();
    }
  }
  controllers_.clear();

  initialized_ = false;
  utils::Logger::info("STM32F4Manager cleanup complete");
}

bool STM32F4Manager::configure_port(uint8_t port_id,
                                    const std::string &uart_device) {
  if (port_id >= STM32F4Controller::NUM_PORTS) {
    return false;
  }

  std::lock_guard<std::mutex> lock(controllers_mutex_);

  controllers_[port_id] =
      std::make_unique<STM32F4Controller>(port_id, uart_device);
  return controllers_[port_id]->initialize();
}

STM32F4Controller *STM32F4Manager::get_controller(uint8_t port_id) {
  if (port_id >= STM32F4Controller::NUM_PORTS) {
    return nullptr;
  }

  std::lock_guard<std::mutex> lock(controllers_mutex_);
  return controllers_[port_id].get();
}

std::vector<STM32F4Reading> STM32F4Manager::read_all_frequencies() {
  std::vector<STM32F4Reading> readings;
  readings.reserve(STM32F4Controller::NUM_PORTS);

  std::lock_guard<std::mutex> lock(controllers_mutex_);
  for (int port_id = 0; port_id < STM32F4Controller::NUM_PORTS; ++port_id) {
    if (controllers_[port_id]) {
      STM32F4Reading reading;
      if (controllers_[port_id]->read_frequency_and_snr(reading)) {
        readings.push_back(reading);
      }
    }
  }

  return readings;
}


bool STM32F4Manager::update_all_displays(const STM32F4DisplayData &data) {
  bool success = true;

  std::lock_guard<std::mutex> lock(controllers_mutex_);
  for (auto &controller : controllers_) {
    if (controller && !controller->update_display(data)) {
      success = false;
    }
  }

  return success;
}

std::vector<uint8_t> STM32F4Manager::get_healthy_ports() const {
  std::vector<uint8_t> healthy_ports;

  std::lock_guard<std::mutex> lock(controllers_mutex_);
  for (int port_id = 0; port_id < STM32F4Controller::NUM_PORTS; ++port_id) {
    if (controllers_[port_id] && controllers_[port_id]->is_healthy()) {
      healthy_ports.push_back(static_cast<uint8_t>(port_id));
    }
  }

  return healthy_ports;
}

std::vector<uint8_t> STM32F4Manager::get_unhealthy_ports() const {
  std::vector<uint8_t> unhealthy_ports;

  std::lock_guard<std::mutex> lock(controllers_mutex_);
  for (int port_id = 0; port_id < STM32F4Controller::NUM_PORTS; ++port_id) {
    if (!controllers_[port_id] || !controllers_[port_id]->is_healthy()) {
      unhealthy_ports.push_back(static_cast<uint8_t>(port_id));
    }
  }

  return unhealthy_ports;
}

bool STM32F4Manager::are_all_ports_healthy() const {
  std::lock_guard<std::mutex> lock(controllers_mutex_);
  for (const auto &controller : controllers_) {
    if (!controller || !controller->is_healthy()) {
      return false;
    }
  }
  return true;
}

bool STM32F4Manager::calibrate_all_ports() {
  bool success = true;

  std::lock_guard<std::mutex> lock(controllers_mutex_);
  for (auto &controller : controllers_) {
    if (controller && !controller->calibrate_frequency_detector()) {
      success = false;
    }
  }

  return success;
}

bool STM32F4Manager::reset_all_mcus() {
  bool success = true;

  std::lock_guard<std::mutex> lock(controllers_mutex_);
  for (auto &controller : controllers_) {
    if (controller && !controller->reset_mcu()) {
      success = false;
    }
  }

  return success;
}

void STM32F4Manager::health_monitor_thread() {
  utils::Logger::debug("STM32F4Manager health monitor thread started");

  while (health_monitoring_) {
    auto unhealthy_ports = get_unhealthy_ports();
    if (!unhealthy_ports.empty()) {
      utils::Logger::warning("Unhealthy STM32F4 ports detected: {}",
                             unhealthy_ports.size());
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
  }

  utils::Logger::debug("STM32F4Manager health monitor thread stopped");
}

} // namespace splitter::hardware