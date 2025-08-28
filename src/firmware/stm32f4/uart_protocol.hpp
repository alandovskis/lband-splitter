#pragma once

#include "frequency_detector.hpp"
#include <array>
#include <cstdint>
#include <functional>

extern "C" {
#include "stm32f4xx_hal.h"
}

namespace stm32f4 {

// Protocol commands (must match host-side definitions)
enum class Command : uint8_t {
  READ_FREQUENCY = 0x01,
  READ_SNR = 0x02,
  SET_LED = 0x03,
  UPDATE_DISPLAY = 0x04,
  GET_STATUS = 0x05,
  CALIBRATE = 0x06,
  RESET = 0x07
};

// Protocol responses
enum class Response : uint8_t {
  OK = 0x00,
  ERROR = 0xFF,
  BUSY = 0xFE,
  INVALID_CMD = 0xFD
};

// Protocol limits
static constexpr size_t MAX_PACKET_SIZE = 64;
static constexpr uint32_t TIMEOUT_MS = 100;

// UART protocol state
enum class UartState { IDLE, RECEIVING_HEADER, RECEIVING_DATA, PROCESSING };

struct CommandPacket {
  Command command;
  uint8_t port_id;
  uint8_t length;
  std::array<uint8_t, MAX_PACKET_SIZE - 3> data;
  uint8_t checksum;
};

struct ResponsePacket {
  Response status;
  uint8_t length;
  std::array<uint8_t, MAX_PACKET_SIZE - 2> data;
};

// LED state structure for SET_LED command
struct LedCommand {
  bool status_led;
  bool signal_led;
  uint8_t brightness;
  bool blinking;
  uint16_t period_ms;
};

// Command handler function types
using FrequencyHandler = std::function<void()>;
using LedHandler = std::function<void(const LedCommand &)>;
using StatusHandler = std::function<void()>;
using CalibrationHandler = std::function<bool()>;
using ResetHandler = std::function<void()>;

class UartProtocol {
public:
  explicit UartProtocol(UART_HandleTypeDef &uart_handle);
  ~UartProtocol() = default;

  bool initialize();
  void process();

  // Response sending methods
  void send_reading(const FrequencyReading &reading);
  void send_response(Response status, const uint8_t *data = nullptr,
                     uint8_t length = 0);

  // Command handler registration
  void set_frequency_handler(FrequencyHandler handler) {
    frequency_handler_ = std::move(handler);
  }
  void set_led_handler(LedHandler handler) {
    led_handler_ = std::move(handler);
  }
  void set_status_handler(StatusHandler handler) {
    status_handler_ = std::move(handler);
  }
  void set_calibration_handler(CalibrationHandler handler) {
    calibration_handler_ = std::move(handler);
  }
  void set_reset_handler(ResetHandler handler) {
    reset_handler_ = std::move(handler);
  }

  // HAL callback for UART receive interrupt
  void uart_rx_complete_callback();

private:
  void process_command(const CommandPacket &cmd);
  void send_packet(const ResponsePacket &packet);
  void reset_uart_state();

  static uint8_t calculate_checksum(const uint8_t *data, uint8_t length);
  static bool verify_checksum(const CommandPacket &packet);

  UART_HandleTypeDef &uart_handle_;
  UartState state_;
  CommandPacket rx_packet_;
  ResponsePacket tx_packet_;
  std::array<uint8_t, MAX_PACKET_SIZE> rx_buffer_;
  uint8_t rx_index_;
  uint8_t expected_length_;
  uint32_t last_activity_;

  uint8_t uart_rx_byte_;
  volatile bool uart_rx_ready_;

  // Command handlers
  FrequencyHandler frequency_handler_;
  LedHandler led_handler_;
  StatusHandler status_handler_;
  CalibrationHandler calibration_handler_;
  ResetHandler reset_handler_;
};

} // namespace stm32f4