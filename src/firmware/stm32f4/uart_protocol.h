#ifndef UART_PROTOCOL_H
#define UART_PROTOCOL_H

#include "frequency_detector.h"
#include <stdbool.h>
#include <stdint.h>

// Protocol commands (must match stm32f4_controller.h)
#define STM32_CMD_READ_FREQUENCY 0x01
#define STM32_CMD_READ_SNR 0x02
#define STM32_CMD_SET_LED 0x03
#define STM32_CMD_UPDATE_DISPLAY 0x04
#define STM32_CMD_GET_STATUS 0x05
#define STM32_CMD_CALIBRATE 0x06
#define STM32_CMD_RESET 0x07

// Protocol responses
#define STM32_RESP_OK 0x00
#define STM32_RESP_ERROR 0xFF
#define STM32_RESP_BUSY 0xFE
#define STM32_RESP_INVALID_CMD 0xFD

// Protocol limits
#define STM32_MAX_PACKET_SIZE 64
#define STM32_TIMEOUT_MS 100

// UART protocol state
typedef enum {
  UART_STATE_IDLE,
  UART_STATE_RECEIVING_HEADER,
  UART_STATE_RECEIVING_DATA,
  UART_STATE_PROCESSING
} UARTState;

typedef struct {
  uint8_t command;
  uint8_t port_id;
  uint8_t length;
  uint8_t data[STM32_MAX_PACKET_SIZE - 3];
  uint8_t checksum;
} CommandPacket;

typedef struct {
  uint8_t status;
  uint8_t length;
  uint8_t data[STM32_MAX_PACKET_SIZE - 2];
} ResponsePacket;

typedef struct {
  UARTState state;
  CommandPacket rx_packet;
  ResponsePacket tx_packet;
  uint8_t rx_buffer[STM32_MAX_PACKET_SIZE];
  uint8_t rx_index;
  uint8_t expected_length;
  uint32_t last_activity;
} UARTProtocolState;

// Public API
void uart_protocol_init(void);
void uart_protocol_process(void);
void uart_protocol_send_reading(const FrequencyReading *reading);
void uart_protocol_send_response(uint8_t status, const uint8_t *data,
                                 uint8_t length);

// Command handlers (to be implemented in main.c)
extern void handle_start_continuous_measurement(void);
extern void handle_stop_continuous_measurement(void);
extern void handle_single_measurement(void);
extern void handle_set_led_state(bool status, bool signal, uint8_t brightness,
                                 bool blinking, uint16_t period);

// Internal functions
static void process_command(const CommandPacket *cmd);
static uint8_t calculate_checksum(const uint8_t *data, uint8_t length);
static bool verify_checksum(const CommandPacket *packet);
static void send_packet(const ResponsePacket *packet);
static void reset_uart_state(void);

#endif // UART_PROTOCOL_H