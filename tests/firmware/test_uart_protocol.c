#include <unity.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Mock UART state
static uint8_t mock_uart_tx_buffer[256];
static uint16_t mock_uart_tx_size = 0;
static bool mock_uart_transmit_success = true;
static uint32_t mock_hal_tick = 0;

// Mock command handler call tracking
static bool mock_single_measurement_called = false;
static bool mock_set_led_called = false;
static bool mock_update_display_called = false;
static double mock_display_frequency = 0.0;
static double mock_display_snr = 0.0;
static bool mock_display_signal_present = false;
static uint8_t mock_display_brightness = 0;
static char mock_display_custom_text[32];

// Mock HAL functions
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, 
                                   uint16_t Size, uint32_t Timeout) {
    if (Size <= sizeof(mock_uart_tx_buffer)) {
        memcpy(mock_uart_tx_buffer, pData, Size);
        mock_uart_tx_size = Size;
    }
    return mock_uart_transmit_success ? HAL_OK : HAL_ERROR;
}

HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, 
                                     uint16_t Size) {
    // Mock implementation - just return success
    return HAL_OK;
}

uint32_t HAL_GetTick(void) {
    return mock_hal_tick;
}

// Mock command handlers
void handle_single_measurement(void) {
    mock_single_measurement_called = true;
}

void handle_set_led_state(bool status, bool signal, uint8_t brightness, 
                         bool blinking, uint16_t period) {
    mock_set_led_called = true;
}

void handle_update_display(double frequency_mhz, double snr_db, bool signal_present, 
                          uint8_t brightness, const char* custom_text) {
    mock_update_display_called = true;
    mock_display_frequency = frequency_mhz;
    mock_display_snr = snr_db;
    mock_display_signal_present = signal_present;
    mock_display_brightness = brightness;
    
    if (custom_text) {
        strncpy(mock_display_custom_text, custom_text, sizeof(mock_display_custom_text) - 1);
        mock_display_custom_text[sizeof(mock_display_custom_text) - 1] = '\0';
    } else {
        mock_display_custom_text[0] = '\0';
    }
}

// Mock frequency detector state
FrequencyDetectorState freq_detector_state = {
    .enabled = true,
    .calibrated = true,
    .measurement_count = 42
};

// Include UART protocol functions
extern void uart_protocol_init(void);
extern void uart_protocol_send_response(uint8_t status, const uint8_t *data, uint8_t length);
extern void process_command(const CommandPacket *cmd);
extern uint8_t calculate_checksum(const uint8_t *data, uint8_t length);

void setUp(void) {
    // Reset mock state
    memset(mock_uart_tx_buffer, 0, sizeof(mock_uart_tx_buffer));
    mock_uart_tx_size = 0;
    mock_uart_transmit_success = true;
    mock_hal_tick = 0;
    
    // Reset command handler call tracking
    mock_single_measurement_called = false;
    mock_set_led_called = false;
    mock_update_display_called = false;
    mock_display_frequency = 0.0;
    mock_display_snr = 0.0;
    mock_display_signal_present = false;
    mock_display_brightness = 0;
    memset(mock_display_custom_text, 0, sizeof(mock_display_custom_text));
}

void tearDown(void) {
    // Clean up after each test
}

void test_uart_protocol_init(void) {
    uart_protocol_init();
    
    // Init should set up the protocol state but not send anything
    TEST_ASSERT_EQUAL_UINT16(0, mock_uart_tx_size);
}

void test_uart_send_response_ok_no_data(void) {
    uart_protocol_send_response(0x00, NULL, 0);
    
    TEST_ASSERT_EQUAL_UINT16(2, mock_uart_tx_size);
    TEST_ASSERT_EQUAL_HEX8(0x00, mock_uart_tx_buffer[0]);  // Status OK
    TEST_ASSERT_EQUAL_HEX8(0x00, mock_uart_tx_buffer[1]);  // Length 0
}

void test_uart_send_response_with_data(void) {
    uint8_t test_data[] = {0x12, 0x34, 0x56, 0x78};
    
    uart_protocol_send_response(0x00, test_data, sizeof(test_data));
    
    TEST_ASSERT_EQUAL_UINT16(6, mock_uart_tx_size);
    TEST_ASSERT_EQUAL_HEX8(0x00, mock_uart_tx_buffer[0]);  // Status OK
    TEST_ASSERT_EQUAL_HEX8(0x04, mock_uart_tx_buffer[1]);  // Length 4
    TEST_ASSERT_EQUAL_HEX8(0x12, mock_uart_tx_buffer[2]);  // Data
    TEST_ASSERT_EQUAL_HEX8(0x34, mock_uart_tx_buffer[3]);
    TEST_ASSERT_EQUAL_HEX8(0x56, mock_uart_tx_buffer[4]);
    TEST_ASSERT_EQUAL_HEX8(0x78, mock_uart_tx_buffer[5]);
}

void test_uart_send_response_error(void) {
    uart_protocol_send_response(0xFF, NULL, 0);
    
    TEST_ASSERT_EQUAL_UINT16(2, mock_uart_tx_size);
    TEST_ASSERT_EQUAL_HEX8(0xFF, mock_uart_tx_buffer[0]);  // Status ERROR
    TEST_ASSERT_EQUAL_HEX8(0x00, mock_uart_tx_buffer[1]);  // Length 0
}

void test_process_command_read_frequency(void) {
    CommandPacket cmd = {
        .command = 0x01,  // STM32_CMD_READ_FREQUENCY
        .port_id = 0,
        .length = 0,
        .checksum = 0x01 ^ 0x00 ^ 0x00  // XOR of command, port_id, length
    };
    
    process_command(&cmd);
    
    TEST_ASSERT_TRUE(mock_single_measurement_called);
}

void test_process_command_set_led(void) {
    CommandPacket cmd = {
        .command = 0x03,  // STM32_CMD_SET_LED
        .port_id = 0,
        .length = 6,
        .data = {1, 0, 255, 1, 0x00, 0x04},  // status=true, signal=false, brightness=255, blinking=true, period=1024ms
        .checksum = 0x03 ^ 0x00 ^ 0x06 ^ 1 ^ 0 ^ 255 ^ 1 ^ 0x00 ^ 0x04
    };
    
    process_command(&cmd);
    
    TEST_ASSERT_TRUE(mock_set_led_called);
    TEST_ASSERT_EQUAL_UINT16(2, mock_uart_tx_size);  // Response sent
    TEST_ASSERT_EQUAL_HEX8(0x00, mock_uart_tx_buffer[0]);  // Status OK
}

void test_process_command_set_led_insufficient_data(void) {
    CommandPacket cmd = {
        .command = 0x03,  // STM32_CMD_SET_LED
        .port_id = 0,
        .length = 4,      // Insufficient data (need 6 bytes)
        .data = {1, 0, 255, 1},
        .checksum = 0x03 ^ 0x00 ^ 0x04 ^ 1 ^ 0 ^ 255 ^ 1
    };
    
    process_command(&cmd);
    
    TEST_ASSERT_FALSE(mock_set_led_called);  // Handler should not be called
    TEST_ASSERT_EQUAL_UINT16(2, mock_uart_tx_size);  // Error response sent
    TEST_ASSERT_EQUAL_HEX8(0xFF, mock_uart_tx_buffer[0]);  // Status ERROR
}

void test_process_command_update_display(void) {
    // Test display update with frequency data
    CommandPacket cmd = {
        .command = 0x04,  // STM32_CMD_UPDATE_DISPLAY
        .port_id = 0,
        .length = 9,
        .data = {
            0x50, 0x04,  // frequency_raw = 1104 -> (1104 * 0.5) + 950 = 1502 MHz
            0x00, 0x02,  // snr_raw = 512 -> (512 * 0.1) - 30 = 21.2 dB
            0x01,        // signal_present = true
            0x50,        // brightness = 80
            0x00, 0x00, 0x00  // padding
        }
    };
    
    // Calculate checksum
    cmd.checksum = cmd.command ^ cmd.port_id ^ cmd.length;
    for (int i = 0; i < cmd.length; i++) {
        cmd.checksum ^= cmd.data[i];
    }
    
    process_command(&cmd);
    
    TEST_ASSERT_TRUE(mock_update_display_called);
    TEST_ASSERT_FLOAT_WITHIN(0.1, 1502.0, mock_display_frequency);
    TEST_ASSERT_FLOAT_WITHIN(0.1, 21.2, mock_display_snr);
    TEST_ASSERT_TRUE(mock_display_signal_present);
    TEST_ASSERT_EQUAL_UINT8(80, mock_display_brightness);
}

void test_process_command_update_display_with_text(void) {
    // Test display update with custom text
    CommandPacket cmd = {
        .command = 0x04,  // STM32_CMD_UPDATE_DISPLAY
        .port_id = 0,
        .length = 15,
        .data = {
            0x00, 0x00,  // frequency_raw = 0
            0x00, 0x00,  // snr_raw = 0
            0x00,        // signal_present = false
            0x64,        // brightness = 100
            'T', 'e', 's', 't', ' ', 'M', 's', 'g', '\0'  // Custom text
        }
    };
    
    // Calculate checksum
    cmd.checksum = cmd.command ^ cmd.port_id ^ cmd.length;
    for (int i = 0; i < cmd.length; i++) {
        cmd.checksum ^= cmd.data[i];
    }
    
    process_command(&cmd);
    
    TEST_ASSERT_TRUE(mock_update_display_called);
    TEST_ASSERT_EQUAL_STRING("Test Msg", mock_display_custom_text);
}

void test_process_command_get_status(void) {
    CommandPacket cmd = {
        .command = 0x05,  // STM32_CMD_GET_STATUS
        .port_id = 0,
        .length = 0,
        .checksum = 0x05 ^ 0x00 ^ 0x00
    };
    
    process_command(&cmd);
    
    TEST_ASSERT_EQUAL_UINT16(6, mock_uart_tx_size);  // Status + length + 4 data bytes
    TEST_ASSERT_EQUAL_HEX8(0x00, mock_uart_tx_buffer[0]);  // Status OK
    TEST_ASSERT_EQUAL_HEX8(0x04, mock_uart_tx_buffer[1]);  // Length 4
    TEST_ASSERT_EQUAL_HEX8(0x01, mock_uart_tx_buffer[2]);  // enabled = true
    TEST_ASSERT_EQUAL_HEX8(0x01, mock_uart_tx_buffer[3]);  // calibrated = true
    TEST_ASSERT_EQUAL_HEX8(0x2A, mock_uart_tx_buffer[4]);  // measurement_count low byte (42)
    TEST_ASSERT_EQUAL_HEX8(0x00, mock_uart_tx_buffer[5]);  // measurement_count high byte
}

void test_process_command_invalid(void) {
    CommandPacket cmd = {
        .command = 0xAB,  // Invalid command
        .port_id = 0,
        .length = 0,
        .checksum = 0xAB ^ 0x00 ^ 0x00
    };
    
    process_command(&cmd);
    
    TEST_ASSERT_EQUAL_UINT16(2, mock_uart_tx_size);
    TEST_ASSERT_EQUAL_HEX8(0xFD, mock_uart_tx_buffer[0]);  // Status INVALID_CMD
    TEST_ASSERT_EQUAL_HEX8(0x00, mock_uart_tx_buffer[1]);  // Length 0
}

void test_calculate_checksum(void) {
    uint8_t test_data[] = {0x12, 0x34, 0x56, 0x78};
    
    uint8_t checksum = calculate_checksum(test_data, sizeof(test_data));
    
    // XOR of all bytes: 0x12 ^ 0x34 ^ 0x56 ^ 0x78 = 0x5A
    TEST_ASSERT_EQUAL_HEX8(0x5A, checksum);
}

void test_calculate_checksum_empty(void) {
    uint8_t checksum = calculate_checksum(NULL, 0);
    TEST_ASSERT_EQUAL_HEX8(0x00, checksum);
}

void test_calculate_checksum_single_byte(void) {
    uint8_t test_data[] = {0xAA};
    
    uint8_t checksum = calculate_checksum(test_data, sizeof(test_data));
    
    TEST_ASSERT_EQUAL_HEX8(0xAA, checksum);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_uart_protocol_init);
    RUN_TEST(test_uart_send_response_ok_no_data);
    RUN_TEST(test_uart_send_response_with_data);
    RUN_TEST(test_uart_send_response_error);
    RUN_TEST(test_process_command_read_frequency);
    RUN_TEST(test_process_command_set_led);
    RUN_TEST(test_process_command_set_led_insufficient_data);
    RUN_TEST(test_process_command_update_display);
    RUN_TEST(test_process_command_update_display_with_text);
    RUN_TEST(test_process_command_get_status);
    RUN_TEST(test_process_command_invalid);
    RUN_TEST(test_calculate_checksum);
    RUN_TEST(test_calculate_checksum_empty);
    RUN_TEST(test_calculate_checksum_single_byte);
    
    return UNITY_END();
}