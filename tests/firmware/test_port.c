#include "unity.h"
#include "../../src/firmware/stm32f4/port.h"
#include "mock_stm32f4xx_hal.h"

// Mock data
static Port test_port;
static uint32_t mock_tick_counter = 0;

// Mock HAL_GetTick
uint32_t HAL_GetTick(void) {
    return mock_tick_counter;
}

// Mock HAL_GPIO_WritePin
void HAL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) {
    // Mock implementation - could track calls if needed
}

void setUp(void) {
    mock_tick_counter = 0;
    port_init(&test_port, 0);
}

void tearDown(void) {
    // Clean up if needed
}

void test_port_initialization(void) {
    TEST_ASSERT_EQUAL(0, test_port.port_id);
    TEST_ASSERT_EQUAL(PORT_STATE_DISABLED, test_port.state);
    TEST_ASSERT_FALSE(test_port.enabled);
    TEST_ASSERT_FALSE(test_port.signal_detected);
    TEST_ASSERT_FALSE(test_port.calculation_active);
    TEST_ASSERT_EQUAL(0.0, test_port.frequency_mhz);
    TEST_ASSERT_EQUAL(0.0, test_port.snr_db);
    TEST_ASSERT_TRUE(test_port.display_showing_frequency);
}

void test_port_enable_disable(void) {
    // Test enabling port
    port_set_enabled(&test_port, true);
    TEST_ASSERT_TRUE(test_port.enabled);
    
    // Test disabling port
    port_set_enabled(&test_port, false);
    TEST_ASSERT_FALSE(test_port.enabled);
    TEST_ASSERT_FALSE(test_port.signal_detected);
    TEST_ASSERT_FALSE(test_port.calculation_active);
}

void test_port_signal_detection(void) {
    // Enable port first
    port_set_enabled(&test_port, true);
    
    // Test signal detection
    port_set_signal_detection(&test_port, true);
    TEST_ASSERT_TRUE(test_port.signal_detected);
    
    // Test signal loss
    port_set_signal_detection(&test_port, false);
    TEST_ASSERT_FALSE(test_port.signal_detected);
    TEST_ASSERT_FALSE(test_port.calculation_active);
}

void test_port_calculation_control(void) {
    port_set_calculation_active(&test_port, true);
    TEST_ASSERT_TRUE(test_port.calculation_active);
    
    port_set_calculation_active(&test_port, false);
    TEST_ASSERT_FALSE(test_port.calculation_active);
}

void test_port_measurement_update(void) {
    double freq = 1575.42;
    double snr = 45.5;
    
    mock_tick_counter = 1000;
    port_update_measurements(&test_port, freq, snr);
    
    TEST_ASSERT_EQUAL_DOUBLE(freq, test_port.frequency_mhz);
    TEST_ASSERT_EQUAL_DOUBLE(snr, test_port.snr_db);
    TEST_ASSERT_EQUAL(1000, test_port.last_update_tick);
}

void test_port_state_transitions(void) {
    // Initial state - disabled
    port_update(&test_port);
    TEST_ASSERT_EQUAL(PORT_STATE_DISABLED, test_port.state);
    
    // Enable port - should be enabled but no signal
    port_set_enabled(&test_port, true);
    port_update(&test_port);
    TEST_ASSERT_EQUAL(PORT_STATE_ENABLED_NO_SIGNAL, test_port.state);
    
    // Start calculation - should be calculating
    port_set_calculation_active(&test_port, true);
    port_update(&test_port);
    TEST_ASSERT_EQUAL(PORT_STATE_ENABLED_CALCULATING, test_port.state);
    
    // Finish calculation and detect signal - should be locked
    port_set_calculation_active(&test_port, false);
    port_set_signal_detection(&test_port, true);
    port_update(&test_port);
    TEST_ASSERT_EQUAL(PORT_STATE_ENABLED_LOCKED, test_port.state);
    
    // Disable port - should be disabled again
    port_set_enabled(&test_port, false);
    port_update(&test_port);
    TEST_ASSERT_EQUAL(PORT_STATE_DISABLED, test_port.state);
}

void test_port_display_toggle_timing(void) {
    // Set up locked state with measurements
    port_set_enabled(&test_port, true);
    port_set_signal_detection(&test_port, true);
    port_update_measurements(&test_port, 1575.42, 45.5);
    
    // Initial state should show frequency
    TEST_ASSERT_TRUE(test_port.display_showing_frequency);
    
    // Advance time by less than toggle interval
    mock_tick_counter = 1000;
    port_update(&test_port);
    TEST_ASSERT_TRUE(test_port.display_showing_frequency);
    
    // Advance time beyond toggle interval (2000ms)
    mock_tick_counter = 3000;
    port_update(&test_port);
    TEST_ASSERT_FALSE(test_port.display_showing_frequency);
    
    // Advance time another interval
    mock_tick_counter = 5000;
    port_update(&test_port);
    TEST_ASSERT_TRUE(test_port.display_showing_frequency);
}

void test_port_gpio_configuration(void) {
    // Test that GPIO configuration is set up correctly
    TEST_ASSERT_NOT_NULL(test_port.status_led_port);
    TEST_ASSERT_NOT_EQUAL(0, test_port.status_led_pin);
    TEST_ASSERT_NOT_NULL(test_port.signal_led_port);
    TEST_ASSERT_NOT_EQUAL(0, test_port.signal_led_pin);
}

void test_port_display_configuration(void) {
    // Test that display configuration is set up correctly
    TEST_ASSERT_EQUAL(0x3C, test_port.display_i2c_address);
    TEST_ASSERT_EQUAL(0, test_port.display_mux_channel);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_port_initialization);
    RUN_TEST(test_port_enable_disable);
    RUN_TEST(test_port_signal_detection);
    RUN_TEST(test_port_calculation_control);
    RUN_TEST(test_port_measurement_update);
    RUN_TEST(test_port_state_transitions);
    RUN_TEST(test_port_display_toggle_timing);
    RUN_TEST(test_port_gpio_configuration);
    RUN_TEST(test_port_display_configuration);
    
    return UNITY_END();
}