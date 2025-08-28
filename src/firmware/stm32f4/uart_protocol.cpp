#include "uart_protocol.hpp"
#include <algorithm>
#include <cstring>

namespace stm32f4 {

UartProtocol::UartProtocol(UART_HandleTypeDef &uart_handle)
    : uart_handle_(uart_handle), state_(UartState::IDLE), rx_index_(0),
      expected_length_(0), last_activity_(0), uart_rx_byte_(0),
      uart_rx_ready_(false) {
  rx_buffer_.fill(0);
}

bool UartProtocol::initialize() {
  state_ = UartState::IDLE;
  rx_index_ = 0;
  expected_length_ = 0;
  last_activity_ = HAL_GetTick();
  uart_rx_ready_ = false;

  // Start receiving first byte
  if (HAL_UART_Receive_IT(&uart_handle_, &uart_rx_byte_, 1) != HAL_OK) {
    return false;
  }

  return true;
}

void UartProtocol::process() {
  // Check for timeout
  uint32_t current_tick = HAL_GetTick();
  if ((current_tick - last_activity_) > TIMEOUT_MS) {
    if (state_ != UartState::IDLE) {
      reset_uart_state();
    }
  }

  // Process received bytes
  if (uart_rx_ready_) {
    uart_rx_ready_ = false;
    last_activity_ = current_tick;

    switch (state_) {
    case UartState::IDLE:
      // Start of new packet - expect command byte
      rx_buffer_[0] = uart_rx_byte_;
      rx_index_ = 1;
      state_ = UartState::RECEIVING_HEADER;
      break;

    case UartState::RECEIVING_HEADER:
      rx_buffer_[rx_index_++] = uart_rx_byte_;

      if (rx_index_ >= 3) {
        // Received command, port_id, and length
        expected_length_ = rx_buffer_[2] + 4; // +4 for header and checksum

        if (expected_length_ > MAX_PACKET_SIZE) {
          // Invalid packet length
          send_response(Response::ERROR);
          reset_uart_state();
        } else if (expected_length_ == 4) {
          // No data payload, just checksum
          state_ = UartState::RECEIVING_DATA;
        } else {
          state_ = UartState::RECEIVING_DATA;
        }
      }
      break;

    case UartState::RECEIVING_DATA:
      rx_buffer_[rx_index_++] = uart_rx_byte_;

      if (rx_index_ >= expected_length_) {
        // Complete packet received
        state_ = UartState::PROCESSING;

        // Parse packet
        rx_packet_.command = static_cast<Command>(rx_buffer_[0]);
        rx_packet_.port_id = rx_buffer_[1];
        rx_packet_.length = rx_buffer_[2];

        if (rx_packet_.length > 0) {
          std::copy(rx_buffer_.begin() + 3,
                    rx_buffer_.begin() + 3 + rx_packet_.length,
                    rx_packet_.data.begin());
        }

        rx_packet_.checksum = rx_buffer_[expected_length_ - 1];

        // Verify checksum
        if (verify_checksum(rx_packet_)) {
          process_command(rx_packet_);
        } else {
          send_response(Response::ERROR);
        }

        reset_uart_state();
      }
      break;

    case UartState::PROCESSING:
    default:
      reset_uart_state();
      break;
    }

    // Continue receiving next byte
    HAL_UART_Receive_IT(&uart_handle_, &uart_rx_byte_, 1);
  }
}

void UartProtocol::send_reading(const FrequencyReading &reading) {
  std::array<uint8_t, 16> data;

  // Pack frequency reading data
  data[0] = reading.raw_frequency & 0xFF;
  data[1] = (reading.raw_frequency >> 8) & 0xFF;
  data[2] = reading.raw_snr & 0xFF;
  data[3] = (reading.raw_snr >> 8) & 0xFF;

  // Pack timestamp (use HAL tick for STM32)
  uint32_t timestamp = HAL_GetTick();
  data[4] = timestamp & 0xFF;
  data[5] = (timestamp >> 8) & 0xFF;
  data[6] = (timestamp >> 16) & 0xFF;
  data[7] = (timestamp >> 24) & 0xFF;

  data[8] = reading.valid ? 1 : 0;
  data[9] = reading.port_id;

  send_response(Response::OK, data.data(), 10);
}

void UartProtocol::send_response(Response status, const uint8_t *data,
                                 uint8_t length) {
  tx_packet_.status = status;
  tx_packet_.length = length;

  if (data && length > 0 && length <= (MAX_PACKET_SIZE - 2)) {
    std::copy(data, data + length, tx_packet_.data.begin());
  } else if (length > 0) {
    // Data too large, send error
    tx_packet_.status = Response::ERROR;
    tx_packet_.length = 0;
  }

  send_packet(tx_packet_);
}

void UartProtocol::process_command(const CommandPacket &cmd) {
  switch (cmd.command) {
  case Command::READ_FREQUENCY:
  case Command::READ_SNR:
    if (frequency_handler_) {
      frequency_handler_();
    } else {
      send_response(Response::ERROR);
    }
    break;

  case Command::SET_LED:
    if (cmd.length >= 6 && led_handler_) {
      LedCommand led_cmd;
      led_cmd.status_led = cmd.data[0] != 0;
      led_cmd.signal_led = cmd.data[1] != 0;
      led_cmd.brightness = cmd.data[2];
      led_cmd.blinking = cmd.data[3] != 0;
      led_cmd.period_ms = cmd.data[4] | (cmd.data[5] << 8);

      led_handler_(led_cmd);
      send_response(Response::OK);
    } else {
      send_response(Response::ERROR);
    }
    break;

  case Command::UPDATE_DISPLAY:
    // Display update command placeholder
    send_response(Response::OK);
    break;

  case Command::GET_STATUS:
    if (status_handler_) {
      status_handler_();
    } else {
      send_response(Response::ERROR);
    }
    break;

  case Command::CALIBRATE:
    if (calibration_handler_) {
      bool success = calibration_handler_();
      send_response(success ? Response::OK : Response::ERROR);
    } else {
      send_response(Response::ERROR);
    }
    break;

  case Command::RESET:
    if (reset_handler_) {
      reset_handler_();
      send_response(Response::OK);
    } else {
      send_response(Response::ERROR);
    }
    break;

  default:
    send_response(Response::INVALID_CMD);
    break;
  }
}

void UartProtocol::send_packet(const ResponsePacket &packet) {
  std::array<uint8_t, MAX_PACKET_SIZE> tx_buffer;
  uint8_t tx_length = 0;

  // Pack response packet
  tx_buffer[tx_length++] = static_cast<uint8_t>(packet.status);
  tx_buffer[tx_length++] = packet.length;

  if (packet.length > 0) {
    std::copy(packet.data.begin(), packet.data.begin() + packet.length,
              tx_buffer.begin() + tx_length);
    tx_length += packet.length;
  }

  // Send via UART
  HAL_UART_Transmit(&uart_handle_, tx_buffer.data(), tx_length, 100);
}

void UartProtocol::reset_uart_state() {
  state_ = UartState::IDLE;
  rx_index_ = 0;
  expected_length_ = 0;
  rx_buffer_.fill(0);
}

void UartProtocol::uart_rx_complete_callback() { uart_rx_ready_ = true; }

uint8_t UartProtocol::calculate_checksum(const uint8_t *data, uint8_t length) {
  uint8_t checksum = 0;
  for (uint8_t i = 0; i < length; ++i) {
    checksum ^= data[i];
  }
  return checksum;
}

bool UartProtocol::verify_checksum(const CommandPacket &packet) {
  uint8_t calculated =
      static_cast<uint8_t>(packet.command) ^ packet.port_id ^ packet.length;

  for (uint8_t i = 0; i < packet.length; ++i) {
    calculated ^= packet.data[i];
  }

  return calculated == packet.checksum;
}

} // namespace stm32f4

// C-style UART callback for HAL integration
extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART2) {
    // In a real implementation, this would access a global or singleton
    // instance For now, this is a placeholder for the callback mechanism
  }
}