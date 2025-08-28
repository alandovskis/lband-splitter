#include "uart_protocol.h"
#include "stm32f4xx_hal.h"
#include <string.h>

// External UART handle (defined in main.c)
extern UART_HandleTypeDef huart2;
extern FrequencyDetectorState freq_detector_state;

// Global protocol state
static UARTProtocolState protocol_state;
static uint8_t uart_rx_byte;
static volatile bool uart_rx_ready = false;

void uart_protocol_init(void) {
  memset(&protocol_state, 0, sizeof(protocol_state));
  protocol_state.state = UART_STATE_IDLE;
  protocol_state.last_activity = HAL_GetTick();
  
  // Start receiving first byte
  HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
}

void uart_protocol_process(void) {
  // Check for timeout
  if ((HAL_GetTick() - protocol_state.last_activity) > STM32_TIMEOUT_MS) {
    if (protocol_state.state != UART_STATE_IDLE) {
      reset_uart_state();
    }
  }
  
  // Process received bytes
  if (uart_rx_ready) {
    uart_rx_ready = false;
    protocol_state.last_activity = HAL_GetTick();
    
    switch (protocol_state.state) {
      case UART_STATE_IDLE:
        // Start of new packet - expect command byte
        protocol_state.rx_buffer[0] = uart_rx_byte;
        protocol_state.rx_index = 1;
        protocol_state.state = UART_STATE_RECEIVING_HEADER;
        break;
        
      case UART_STATE_RECEIVING_HEADER:
        protocol_state.rx_buffer[protocol_state.rx_index++] = uart_rx_byte;
        
        if (protocol_state.rx_index >= 3) {
          // Received command, port_id, and length
          protocol_state.expected_length = protocol_state.rx_buffer[2] + 4; // +4 for header and checksum
          
          if (protocol_state.expected_length > STM32_MAX_PACKET_SIZE) {
            // Invalid packet length
            uart_protocol_send_response(STM32_RESP_ERROR, NULL, 0);
            reset_uart_state();
          } else if (protocol_state.expected_length == 4) {
            // No data payload, just checksum
            protocol_state.state = UART_STATE_RECEIVING_DATA;
          } else {
            protocol_state.state = UART_STATE_RECEIVING_DATA;
          }
        }
        break;
        
      case UART_STATE_RECEIVING_DATA:
        protocol_state.rx_buffer[protocol_state.rx_index++] = uart_rx_byte;
        
        if (protocol_state.rx_index >= protocol_state.expected_length) {
          // Complete packet received
          protocol_state.state = UART_STATE_PROCESSING;
          
          // Parse packet
          CommandPacket *cmd = &protocol_state.rx_packet;
          cmd->command = protocol_state.rx_buffer[0];
          cmd->port_id = protocol_state.rx_buffer[1];
          cmd->length = protocol_state.rx_buffer[2];
          
          if (cmd->length > 0) {
            memcpy(cmd->data, &protocol_state.rx_buffer[3], cmd->length);
          }
          
          cmd->checksum = protocol_state.rx_buffer[protocol_state.expected_length - 1];
          
          // Verify checksum
          if (verify_checksum(cmd)) {
            process_command(cmd);
          } else {
            uart_protocol_send_response(STM32_RESP_ERROR, NULL, 0);
          }
          
          reset_uart_state();
        }
        break;
        
      default:
        reset_uart_state();
        break;
    }
    
    // Continue receiving next byte
    HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
  }
}

void uart_protocol_send_reading(const FrequencyReading *reading) {
  uint8_t data[16];
  
  // Pack frequency reading data
  data[0] = reading->raw_frequency & 0xFF;
  data[1] = (reading->raw_frequency >> 8) & 0xFF;
  data[2] = reading->raw_snr & 0xFF;
  data[3] = (reading->raw_snr >> 8) & 0xFF;
  data[4] = reading->timestamp & 0xFF;
  data[5] = (reading->timestamp >> 8) & 0xFF;
  data[6] = (reading->timestamp >> 16) & 0xFF;
  data[7] = (reading->timestamp >> 24) & 0xFF;
  data[8] = reading->valid ? 1 : 0;
  data[9] = reading->port_id;
  
  uart_protocol_send_response(STM32_RESP_OK, data, 10);
}

void uart_protocol_send_response(uint8_t status, const uint8_t *data, uint8_t length) {
  ResponsePacket *resp = &protocol_state.tx_packet;
  
  resp->status = status;
  resp->length = length;
  
  if (data && length > 0 && length <= (STM32_MAX_PACKET_SIZE - 2)) {
    memcpy(resp->data, data, length);
  } else if (length > 0) {
    // Data too large, send error
    resp->status = STM32_RESP_ERROR;
    resp->length = 0;
  }
  
  send_packet(resp);
}

static void process_command(const CommandPacket *cmd) {
  switch (cmd->command) {
    case STM32_CMD_READ_FREQUENCY:
      handle_single_measurement();
      break;
      
    case STM32_CMD_READ_SNR:
      // SNR is included in frequency reading
      handle_single_measurement();
      break;
      
    case STM32_CMD_SET_LED:
      if (cmd->length >= 6) {
        // Extract LED state from command data
        bool status_led = cmd->data[0];
        bool signal_led = cmd->data[1];
        uint8_t brightness = cmd->data[2];
        bool blinking = cmd->data[3];
        uint16_t period = cmd->data[4] | (cmd->data[5] << 8);
        
        // Set LED state via main.c handler
        handle_set_led_state(status_led, signal_led, brightness, blinking, period);
        uart_protocol_send_response(STM32_RESP_OK, NULL, 0);
      } else {
        uart_protocol_send_response(STM32_RESP_ERROR, NULL, 0);
      }
      break;
      
    case STM32_CMD_UPDATE_DISPLAY:
      // Display update command (would control small LCD/OLED here)
      uart_protocol_send_response(STM32_RESP_OK, NULL, 0);
      break;
      
    case STM32_CMD_GET_STATUS:
      {
        uint8_t status_data[4];
        status_data[0] = freq_detector_state.enabled ? 1 : 0;
        status_data[1] = freq_detector_state.calibrated ? 1 : 0;
        status_data[2] = (freq_detector_state.measurement_count >> 0) & 0xFF;
        status_data[3] = (freq_detector_state.measurement_count >> 8) & 0xFF;
        
        uart_protocol_send_response(STM32_RESP_OK, status_data, 4);
      }
      break;
      
    case STM32_CMD_CALIBRATE:
      if (frequency_detector_calibrate(&freq_detector_state)) {
        uart_protocol_send_response(STM32_RESP_OK, NULL, 0);
      } else {
        uart_protocol_send_response(STM32_RESP_ERROR, NULL, 0);
      }
      break;
      
    case STM32_CMD_RESET:
      frequency_detector_reset(&freq_detector_state);
      uart_protocol_send_response(STM32_RESP_OK, NULL, 0);
      break;
      
    default:
      uart_protocol_send_response(STM32_RESP_INVALID_CMD, NULL, 0);
      break;
  }
}

static uint8_t calculate_checksum(const uint8_t *data, uint8_t length) {
  uint8_t checksum = 0;
  for (uint8_t i = 0; i < length; i++) {
    checksum ^= data[i];
  }
  return checksum;
}

static bool verify_checksum(const CommandPacket *packet) {
  uint8_t calculated = packet->command ^ packet->port_id ^ packet->length;
  
  for (uint8_t i = 0; i < packet->length; i++) {
    calculated ^= packet->data[i];
  }
  
  return calculated == packet->checksum;
}

static void send_packet(const ResponsePacket *packet) {
  uint8_t tx_buffer[STM32_MAX_PACKET_SIZE];
  uint8_t tx_length = 0;
  
  // Pack response packet
  tx_buffer[tx_length++] = packet->status;
  tx_buffer[tx_length++] = packet->length;
  
  if (packet->length > 0) {
    memcpy(&tx_buffer[tx_length], packet->data, packet->length);
    tx_length += packet->length;
  }
  
  // Send via UART
  HAL_UART_Transmit(&huart2, tx_buffer, tx_length, 100);
}

static void reset_uart_state(void) {
  protocol_state.state = UART_STATE_IDLE;
  protocol_state.rx_index = 0;
  protocol_state.expected_length = 0;
  memset(protocol_state.rx_buffer, 0, sizeof(protocol_state.rx_buffer));
}

// UART interrupt callback
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART2) {
    uart_rx_ready = true;
  }
}