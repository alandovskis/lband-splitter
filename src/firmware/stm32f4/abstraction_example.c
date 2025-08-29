/**
 * @file abstraction_example.c
 * @brief Example showing how to use the display and LED abstractions
 * 
 * This file demonstrates the proper usage of the new hardware abstraction
 * layers for displays and LEDs in the STM32F4 firmware.
 */

#include "display_abstraction.h"
#include "led_abstraction.h"
#include "stm32f4xx_hal.h"

// Example I2C handle (would be initialized in main.c)
extern I2C_HandleTypeDef hi2c1;

/**
 * @brief Example function showing how to initialize and use display abstractions
 */
void example_display_usage(void) {
    // Initialize the display abstraction system
    display_abstraction_init();
    
    // Create a display for port 0 with I2C multiplexer
    DisplayAbstraction* display = display_create_multiplexed(
        0,              // port_id
        DISPLAY_TYPE_OLED_SSD1306,  // display type
        0x3C,           // display I2C address
        0x70,           // multiplexer address
        0,              // multiplexer channel
        &hi2c1          // I2C handle
    );
    
    if (display) {
        // Initialize the display hardware
        if (display_initialize(display)) {
            // Clear the display
            display_clear_screen(display);
            
            // Show some text
            display_show_text(display, "Hello, World!");
            
            // Show two lines
            display_show_two_lines(display, "Port 1", "1575.42 MHz");
            
            // Show frequency with helper function
            display_show_frequency(display, 1575.42);
            
            // Enable/disable display
            display_set_enabled(display, true);
        }
        
        // Clean up when done
        display_destroy(display);
    }
}

/**
 * @brief Example function showing how to initialize and use LED abstractions
 */
void example_led_usage(void) {
    // Initialize the LED abstraction system
    led_abstraction_init();
    
    // Create a simple status LED
    LedAbstraction* status_led = led_create_simple(
        0,              // led_id
        0,              // port_id
        LED_TYPE_STATUS,// LED type
        GPIOA,          // GPIO port
        GPIO_PIN_0,     // GPIO pin
        true            // active high
    );
    
    if (status_led) {
        // Initialize the LED hardware
        if (led_initialize(status_led)) {
            // Turn LED on
            led_set_state(status_led, LED_STATE_ON);
            HAL_Delay(1000);
            
            // Turn LED off
            led_set_state(status_led, LED_STATE_OFF);
            HAL_Delay(1000);
            
            // Start slow blinking
            led_set_state(status_led, LED_STATE_BLINK_SLOW);
            
            // In main loop, call led_update() regularly to handle patterns
            for (int i = 0; i < 100; i++) {
                led_update(status_led);
                HAL_Delay(50);
            }
            
            // Flash the LED 3 times
            led_flash_count(status_led, 3);
            while (led_is_active(status_led)) {
                led_update(status_led);
                HAL_Delay(50);
            }
        }
        
        // Clean up when done
        led_destroy(status_led);
    }
}

/**
 * @brief Example function showing how to use port LED groups
 */
void example_port_led_group_usage(void) {
    // Initialize LED abstraction system
    led_abstraction_init();
    
    // Create a LED group for port 0
    PortLedGroup* port_leds = port_leds_create(0);
    
    if (port_leds) {
        // Create individual LEDs for the port
        port_leds->status_led = led_create_simple(
            0, 0, LED_TYPE_STATUS, GPIOA, GPIO_PIN_0, true
        );
        
        port_leds->signal_led = led_create_simple(
            1, 0, LED_TYPE_SIGNAL, GPIOB, GPIO_PIN_0, true
        );
        
        // Initialize LEDs
        if (port_leds->status_led) led_initialize(port_leds->status_led);
        if (port_leds->signal_led) led_initialize(port_leds->signal_led);
        
        // Use high-level port LED functions
        port_leds_set_status(port_leds, true);           // Port enabled
        port_leds_set_signal_detected(port_leds, false); // No signal
        port_leds_set_calculating(port_leds, true);      // Calculating
        
        // Update patterns in main loop
        for (int i = 0; i < 200; i++) {
            port_leds_update_all(port_leds);
            HAL_Delay(50);
        }
        
        // Signal detected
        port_leds_set_signal_detected(port_leds, true);
        port_leds_set_calculating(port_leds, false);
        
        // Update for another few seconds
        for (int i = 0; i < 100; i++) {
            port_leds_update_all(port_leds);
            HAL_Delay(50);
        }
        
        // Clean up
        port_leds_destroy(port_leds);
    }
}

/**
 * @brief Example showing integrated port usage (the recommended way)
 */
void example_integrated_port_usage(void) {
    #include "port.h"
    
    Port port;
    
    // Initialize port structure
    port_init(&port, 0);
    
    // Initialize hardware abstractions
    if (port_init_hardware(&port, &hi2c1)) {
        // Enable the port
        port_set_enabled(&port, true);
        
        // Simulate no signal initially
        port_set_signal_detection(&port, false);
        port_update(&port);
        HAL_Delay(2000);
        
        // Simulate signal detected, start calculating
        port_set_signal_detection(&port, true);
        port_set_calculation_active(&port, true);
        port_update(&port);
        HAL_Delay(3000);
        
        // Simulation calculation complete with measurements
        port_set_calculation_active(&port, false);
        port_update_measurements(&port, 1575.42, 45.2); // GPS L1 frequency, good SNR
        port_update(&port);
        HAL_Delay(5000);
        
        // Clean up hardware
        port_cleanup_hardware(&port);
    }
}

/**
 * @brief Example showing advanced LED patterns
 */
void example_advanced_led_patterns(void) {
    led_abstraction_init();
    
    // Create RGB LED (if available)
    LedAbstraction* rgb_led = led_create_rgb(
        10,             // led_id
        0,              // port_id
        GPIOA, GPIO_PIN_1,  // Red
        GPIOA, GPIO_PIN_2,  // Green
        GPIOA, GPIO_PIN_3   // Blue
    );
    
    if (rgb_led) {
        led_initialize(rgb_led);
        
        // Show different colors
        led_set_color(rgb_led, 255, 0, 0);      // Red
        HAL_Delay(1000);
        led_set_color(rgb_led, 0, 255, 0);      // Green
        HAL_Delay(1000);
        led_set_color(rgb_led, 0, 0, 255);      // Blue
        HAL_Delay(1000);
        led_set_color(rgb_led, 255, 255, 255);  // White
        HAL_Delay(1000);
        
        led_destroy(rgb_led);
    }
    
    // Custom timing configuration
    LedAbstraction* custom_led = led_create_simple(
        11, 0, LED_TYPE_ACTIVITY, GPIOB, GPIO_PIN_1, true
    );
    
    if (custom_led) {
        led_initialize(custom_led);
        
        // Configure custom timing
        LedTimingConfig custom_timing = {
            .slow_blink_period_ms = 2000,   // 2 second slow blink
            .fast_blink_period_ms = 100,    // Very fast blink
            .pulse_period_ms = 3000,        // 3 second pulse cycle
            .flash_duration_ms = 50,        // Short flash
            .pulse_min_brightness = 5,      // Very dim minimum
            .pulse_max_brightness = 100     // Full brightness maximum
        };
        
        led_configure_timing(custom_led, &custom_timing);
        
        // Test different patterns with custom timing
        led_set_state(custom_led, LED_STATE_BLINK_FAST);
        for (int i = 0; i < 100; i++) {
            led_update(custom_led);
            HAL_Delay(50);
        }
        
        led_destroy(custom_led);
    }
}