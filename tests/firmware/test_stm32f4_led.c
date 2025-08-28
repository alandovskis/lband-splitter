#include <unity.h>
#include <stdint.h>
#include <stdbool.h>

// Mock GPIO state tracking
static bool mock_gpio_pa5_state = false;  // Status LED
static bool mock_gpio_pa6_state = false;  // Signal LED
static uint32_t mock_hal_tick = 0;

// Mock HAL_GPIO_WritePin function
void HAL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) {
    if (GPIOx == GPIOA) {
        if (GPIO_Pin == GPIO_PIN_5) {
            mock_gpio_pa5_state = (PinState == GPIO_PIN_SET);
        } else if (GPIO_Pin == GPIO_PIN_6) {
            mock_gpio_pa6_state = (PinState == GPIO_PIN_SET);
        }
    }
}

// Mock HAL_GetTick function
uint32_t HAL_GetTick(void) {
    return mock_hal_tick;
}

// Include the LED functions from main.c (would be in separate file in real implementation)
extern void set_status_led(bool on);
extern void set_signal_led(bool on);
extern void handle_led_blinking(void);
extern void handle_set_led_state(bool status, bool signal, uint8_t brightness, bool blinking, uint16_t period);

// Access to static variables (would be exposed via getters in real implementation)
extern volatile bool status_led_on;
extern volatile bool signal_led_on;
extern volatile bool led_blinking;
extern volatile uint16_t blink_period_ms;
extern volatile uint32_t last_blink_time;

void setUp(void) {
    // Reset mock state
    mock_gpio_pa5_state = false;
    mock_gpio_pa6_state = false;
    mock_hal_tick = 0;
    
    // Reset LED state
    status_led_on = false;
    signal_led_on = false;
    led_blinking = false;
    blink_period_ms = 1000;
    last_blink_time = 0;
}

void tearDown(void) {
    // Clean up after each test
}

void test_set_status_led_on(void) {
    set_status_led(true);
    
    TEST_ASSERT_TRUE(mock_gpio_pa5_state);
    TEST_ASSERT_TRUE(status_led_on);
}

void test_set_status_led_off(void) {
    // First turn on
    set_status_led(true);
    TEST_ASSERT_TRUE(mock_gpio_pa5_state);
    
    // Then turn off
    set_status_led(false);
    TEST_ASSERT_FALSE(mock_gpio_pa5_state);
    TEST_ASSERT_FALSE(status_led_on);
}

void test_set_signal_led_on(void) {
    set_signal_led(true);
    
    TEST_ASSERT_TRUE(mock_gpio_pa6_state);
    TEST_ASSERT_TRUE(signal_led_on);
}

void test_set_signal_led_off(void) {
    // First turn on
    set_signal_led(true);
    TEST_ASSERT_TRUE(mock_gpio_pa6_state);
    
    // Then turn off
    set_signal_led(false);
    TEST_ASSERT_FALSE(mock_gpio_pa6_state);
    TEST_ASSERT_FALSE(signal_led_on);
}

void test_handle_set_led_state_static(void) {
    // Test static LED state (no blinking)
    handle_set_led_state(true, false, 255, false, 1000);
    
    TEST_ASSERT_TRUE(mock_gpio_pa5_state);   // Status LED on
    TEST_ASSERT_FALSE(mock_gpio_pa6_state);  // Signal LED off
    TEST_ASSERT_FALSE(led_blinking);
}

void test_handle_set_led_state_blinking_setup(void) {
    mock_hal_tick = 1000;
    
    // Test blinking LED state setup
    handle_set_led_state(true, true, 255, true, 500);
    
    TEST_ASSERT_TRUE(led_blinking);
    TEST_ASSERT_EQUAL_UINT16(500, blink_period_ms);
    TEST_ASSERT_EQUAL_UINT32(1000, last_blink_time);
}

void test_handle_led_blinking_no_blink(void) {
    // Setup LEDs in static state
    set_status_led(true);
    set_signal_led(false);
    led_blinking = false;
    
    bool initial_status = mock_gpio_pa5_state;
    bool initial_signal = mock_gpio_pa6_state;
    
    handle_led_blinking();
    
    // LEDs should not change when blinking is disabled
    TEST_ASSERT_EQUAL(initial_status, mock_gpio_pa5_state);
    TEST_ASSERT_EQUAL(initial_signal, mock_gpio_pa6_state);
}

void test_handle_led_blinking_not_time_yet(void) {
    // Setup blinking state
    led_blinking = true;
    blink_period_ms = 1000;
    last_blink_time = 500;
    mock_hal_tick = 1000;  // Only 500ms elapsed, not enough for 1000ms period
    
    set_status_led(true);
    set_signal_led(false);
    
    bool initial_status = mock_gpio_pa5_state;
    bool initial_signal = mock_gpio_pa6_state;
    
    handle_led_blinking();
    
    // LEDs should not toggle yet
    TEST_ASSERT_EQUAL(initial_status, mock_gpio_pa5_state);
    TEST_ASSERT_EQUAL(initial_signal, mock_gpio_pa6_state);
}

void test_handle_led_blinking_toggle_time(void) {
    // Setup blinking state
    led_blinking = true;
    blink_period_ms = 1000;
    last_blink_time = 500;
    mock_hal_tick = 1500;  // 1000ms elapsed, time to toggle
    
    set_status_led(true);
    set_signal_led(false);
    
    handle_led_blinking();
    
    // LEDs should toggle
    TEST_ASSERT_FALSE(mock_gpio_pa5_state);  // Status LED toggled off
    TEST_ASSERT_TRUE(mock_gpio_pa6_state);   // Signal LED toggled on
    TEST_ASSERT_EQUAL_UINT32(1500, last_blink_time);  // Time updated
}

void test_handle_led_blinking_multiple_toggles(void) {
    // Test multiple blink cycles
    led_blinking = true;
    blink_period_ms = 500;
    last_blink_time = 0;
    
    // Initial state: both off
    set_status_led(false);
    set_signal_led(false);
    
    // First toggle at 500ms
    mock_hal_tick = 500;
    handle_led_blinking();
    TEST_ASSERT_TRUE(mock_gpio_pa5_state);   // Status LED on
    TEST_ASSERT_TRUE(mock_gpio_pa6_state);   // Signal LED on
    
    // Second toggle at 1000ms
    mock_hal_tick = 1000;
    handle_led_blinking();
    TEST_ASSERT_FALSE(mock_gpio_pa5_state);  // Status LED off
    TEST_ASSERT_FALSE(mock_gpio_pa6_state);  // Signal LED off
    
    // Third toggle at 1500ms
    mock_hal_tick = 1500;
    handle_led_blinking();
    TEST_ASSERT_TRUE(mock_gpio_pa5_state);   // Status LED on again
    TEST_ASSERT_TRUE(mock_gpio_pa6_state);   // Signal LED on again
}

void test_led_blink_period_variations(void) {
    // Test different blink periods
    uint16_t test_periods[] = {100, 250, 500, 1000, 2000};
    size_t num_periods = sizeof(test_periods) / sizeof(test_periods[0]);
    
    for (size_t i = 0; i < num_periods; i++) {
        setUp();  // Reset state
        
        led_blinking = true;
        blink_period_ms = test_periods[i];
        last_blink_time = 0;
        set_status_led(false);
        set_signal_led(false);
        
        // Test that LEDs don't toggle before period expires
        mock_hal_tick = test_periods[i] - 1;
        handle_led_blinking();
        TEST_ASSERT_FALSE(mock_gpio_pa5_state);
        TEST_ASSERT_FALSE(mock_gpio_pa6_state);
        
        // Test that LEDs toggle when period expires
        mock_hal_tick = test_periods[i];
        handle_led_blinking();
        TEST_ASSERT_TRUE(mock_gpio_pa5_state);
        TEST_ASSERT_TRUE(mock_gpio_pa6_state);
    }
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_set_status_led_on);
    RUN_TEST(test_set_status_led_off);
    RUN_TEST(test_set_signal_led_on);
    RUN_TEST(test_set_signal_led_off);
    RUN_TEST(test_handle_set_led_state_static);
    RUN_TEST(test_handle_set_led_state_blinking_setup);
    RUN_TEST(test_handle_led_blinking_no_blink);
    RUN_TEST(test_handle_led_blinking_not_time_yet);
    RUN_TEST(test_handle_led_blinking_toggle_time);
    RUN_TEST(test_handle_led_blinking_multiple_toggles);
    RUN_TEST(test_led_blink_period_variations);
    
    return UNITY_END();
}