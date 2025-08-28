#include <unity.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Mock STM32 HAL functions
static bool mock_i2c_transmit_success = true;
static uint8_t mock_i2c_last_address = 0;
static uint8_t mock_i2c_last_data[128];
static uint16_t mock_i2c_last_size = 0;

// Mock HAL_I2C_Transmit function
HAL_StatusTypeDef HAL_I2C_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, 
                                  uint8_t *pData, uint16_t Size, uint32_t Timeout) {
    mock_i2c_last_address = DevAddress;
    if (Size <= sizeof(mock_i2c_last_data)) {
        memcpy(mock_i2c_last_data, pData, Size);
        mock_i2c_last_size = Size;
    }
    return mock_i2c_transmit_success ? HAL_OK : HAL_ERROR;
}

// Include the display functions from main.c (would be in separate file in real implementation)
extern void display_init(void);
extern void display_clear(void);
extern void display_update_frequency_data(double frequency_mhz, double snr_db, bool signal_present);
extern void display_set_brightness(uint8_t brightness);
extern void display_show_custom_text(const char* text);

void setUp(void) {
    // Reset mock state
    mock_i2c_transmit_success = true;
    mock_i2c_last_address = 0;
    memset(mock_i2c_last_data, 0, sizeof(mock_i2c_last_data));
    mock_i2c_last_size = 0;
}

void tearDown(void) {
    // Clean up after each test
}

void test_display_init_success(void) {
    mock_i2c_transmit_success = true;
    
    display_init();
    
    // Verify I2C communication occurred
    TEST_ASSERT_EQUAL_HEX16(0x3C << 1, mock_i2c_last_address);  // SSD1306 address
    TEST_ASSERT_TRUE(mock_i2c_last_size > 0);
    
    // Verify initialization commands were sent
    TEST_ASSERT_EQUAL_HEX8(0x00, mock_i2c_last_data[0]);  // Command mode
    TEST_ASSERT_EQUAL_HEX8(0xAE, mock_i2c_last_data[1]);  // Display OFF command
}

void test_display_init_failure(void) {
    mock_i2c_transmit_success = false;
    
    display_init();
    
    // Verify I2C communication was attempted but failed
    TEST_ASSERT_EQUAL_HEX16(0x3C << 1, mock_i2c_last_address);
}

void test_display_set_brightness(void) {
    mock_i2c_transmit_success = true;
    
    // Initialize display first
    display_init();
    
    // Test brightness setting
    display_set_brightness(50);
    
    // Verify brightness command was sent
    TEST_ASSERT_EQUAL_HEX16(0x3C << 1, mock_i2c_last_address);
    TEST_ASSERT_EQUAL_HEX8(0x00, mock_i2c_last_data[0]);  // Command mode
    TEST_ASSERT_EQUAL_HEX8(0x81, mock_i2c_last_data[1]);  // Set contrast command
    TEST_ASSERT_EQUAL_HEX8(127, mock_i2c_last_data[2]);   // 50% brightness = 127
}

void test_display_update_frequency_data_with_signal(void) {
    mock_i2c_transmit_success = true;
    
    // Initialize display first
    display_init();
    
    // Test frequency display with valid signal
    display_update_frequency_data(1575.42, 25.5, true);
    
    // Verify I2C communication occurred (text updates)
    TEST_ASSERT_EQUAL_HEX16(0x3C << 1, mock_i2c_last_address);
    TEST_ASSERT_TRUE(mock_i2c_last_size > 0);
}

void test_display_update_frequency_data_no_signal(void) {
    mock_i2c_transmit_success = true;
    
    // Initialize display first
    display_init();
    
    // Test frequency display with no signal
    display_update_frequency_data(0.0, -50.0, false);
    
    // Verify I2C communication occurred (should show dashes)
    TEST_ASSERT_EQUAL_HEX16(0x3C << 1, mock_i2c_last_address);
    TEST_ASSERT_TRUE(mock_i2c_last_size > 0);
}

void test_display_show_custom_text(void) {
    mock_i2c_transmit_success = true;
    
    // Initialize display first
    display_init();
    
    // Test custom text display
    const char* test_text = "Test Message";
    display_show_custom_text(test_text);
    
    // Verify I2C communication occurred
    TEST_ASSERT_EQUAL_HEX16(0x3C << 1, mock_i2c_last_address);
    TEST_ASSERT_TRUE(mock_i2c_last_size > 0);
}

void test_display_clear(void) {
    mock_i2c_transmit_success = true;
    
    // Initialize display first
    display_init();
    
    // Test display clear
    display_clear();
    
    // Verify clear commands were sent
    TEST_ASSERT_EQUAL_HEX16(0x3C << 1, mock_i2c_last_address);
    TEST_ASSERT_TRUE(mock_i2c_last_size > 0);
}

void test_display_operations_without_init(void) {
    // Test that display operations don't crash when display not initialized
    display_set_brightness(50);
    display_update_frequency_data(1575.42, 25.5, true);
    display_show_custom_text("Test");
    display_clear();
    
    // No I2C communication should occur
    TEST_ASSERT_EQUAL_HEX16(0, mock_i2c_last_address);
    TEST_ASSERT_EQUAL_UINT16(0, mock_i2c_last_size);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_display_init_success);
    RUN_TEST(test_display_init_failure);
    RUN_TEST(test_display_set_brightness);
    RUN_TEST(test_display_update_frequency_data_with_signal);
    RUN_TEST(test_display_update_frequency_data_no_signal);
    RUN_TEST(test_display_show_custom_text);
    RUN_TEST(test_display_clear);
    RUN_TEST(test_display_operations_without_init);
    
    return UNITY_END();
}