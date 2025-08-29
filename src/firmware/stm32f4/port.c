#include "port.h"
#include "display_abstraction.h"
#include "led_abstraction.h"
#include <stdio.h>

// Constants
#define DISPLAY_TOGGLE_INTERVAL_MS 2000

// External I2C handle (should be defined in main.c)
extern I2C_HandleTypeDef hi2c1;

// Helper functions to get GPIO assignments for each port
static GPIO_TypeDef* get_status_led_port(uint8_t port_id) {
    return (port_id < 16) ? GPIOA : GPIOC;
}

static uint16_t get_status_led_pin(uint8_t port_id) {
    return 1 << (port_id % 16);
}

static GPIO_TypeDef* get_signal_led_port(uint8_t port_id) {
    return (port_id < 16) ? GPIOB : GPIOD;
}

static uint16_t get_signal_led_pin(uint8_t port_id) {
    return 1 << (port_id % 16);
}

void port_init(Port* port, uint8_t port_id) {
    port->port_id = port_id;
    port->state = PORT_STATE_DISABLED;
    port->enabled = false;
    port->signal_detected = false;
    port->calculation_active = false;
    
    port->frequency_mhz = 0.0;
    port->snr_db = 0.0;
    port->last_update_tick = 0;
    
    // Initialize hardware abstractions to NULL
    port->display = NULL;
    port->leds = NULL;
    
    // Display state
    port->display_showing_frequency = true;
    port->display_toggle_tick = 0;
}

bool port_init_hardware(Port* port, I2C_HandleTypeDef* hi2c) {
    if (!port) {
        return false;
    }
    
    // Initialize display abstraction
    // Each port has its own display on I2C multiplexer
    uint8_t mux_address = 0x70; // TCA9548A multiplexer address
    uint8_t display_address = 0x3C; // SSD1306 OLED address
    uint8_t mux_channel = port->port_id % 8; // 8 channels per multiplexer
    
    port->display = display_create_multiplexed(port->port_id, DISPLAY_TYPE_OLED_SSD1306,
                                              display_address, mux_address, mux_channel, hi2c);
    
    if (port->display) {
        display_initialize(port->display);
        display_clear_screen(port->display);
    }
    
    // Initialize LED group for this port
    port->leds = port_leds_create(port->port_id);
    
    if (port->leds) {
        // Create status LED
        port->leds->status_led = led_create_simple(
            port->port_id * 4 + 0,  // Unique LED ID
            port->port_id,
            LED_TYPE_STATUS,
            get_status_led_port(port->port_id),
            get_status_led_pin(port->port_id),
            true  // Active high
        );
        
        // Create signal LED
        port->leds->signal_led = led_create_simple(
            port->port_id * 4 + 1,  // Unique LED ID
            port->port_id,
            LED_TYPE_SIGNAL,
            get_signal_led_port(port->port_id),
            get_signal_led_pin(port->port_id),
            true  // Active high
        );
        
        // Initialize LEDs
        if (port->leds->status_led) {
            led_initialize(port->leds->status_led);
            led_set_state(port->leds->status_led, LED_STATE_OFF);
        }
        
        if (port->leds->signal_led) {
            led_initialize(port->leds->signal_led);
            led_set_state(port->leds->signal_led, LED_STATE_OFF);
        }
    }
    
    return (port->display != NULL && port->leds != NULL);
}

void port_cleanup_hardware(Port* port) {
    if (!port) {
        return;
    }
    
    // Cleanup display
    if (port->display) {
        display_destroy(port->display);
        port->display = NULL;
    }
    
    // Cleanup LEDs
    if (port->leds) {
        port_leds_destroy(port->leds);
        port->leds = NULL;
    }
}

void port_update(Port* port) {
    if (!port) {
        return;
    }
    
    // Update port state based on current conditions
    if (!port->enabled) {
        port->state = PORT_STATE_DISABLED;
    } else if (!port->signal_detected) {
        port->state = PORT_STATE_ENABLED_NO_SIGNAL;
    } else if (port->calculation_active) {
        port->state = PORT_STATE_ENABLED_CALCULATING;
    } else {
        port->state = PORT_STATE_ENABLED_LOCKED;
    }
    
    // Update LEDs based on state
    port_update_leds(port);
    
    // Update display based on state
    port_update_display(port);
    
    // Update LED patterns
    if (port->leds) {
        port_leds_update_all(port->leds);
    }
}

void port_set_enabled(Port* port, bool enabled) {
    if (!port) {
        return;
    }
    
    port->enabled = enabled;
    
    // Update status LED
    if (port->leds) {
        port_leds_set_status(port->leds, enabled);
    }
    
    // Clear display if disabled
    if (!enabled && port->display) {
        display_clear_screen(port->display);
    }
}

void port_set_signal_detection(Port* port, bool detected) {
    if (!port) {
        return;
    }
    
    port->signal_detected = detected;
    
    // Update signal LED
    if (port->leds) {
        port_leds_set_signal_detected(port->leds, detected);
    }
}

void port_set_calculation_active(Port* port, bool active) {
    if (!port) {
        return;
    }
    
    port->calculation_active = active;
    
    // Update calculation indicator (could use activity LED if available)
    if (port->leds && port->leds->activity_led) {
        port_leds_set_calculating(port->leds, active);
    }
}

void port_update_measurements(Port* port, double frequency_mhz, double snr_db) {
    if (!port) {
        return;
    }
    
    port->frequency_mhz = frequency_mhz;
    port->snr_db = snr_db;
    port->last_update_tick = HAL_GetTick();
    
    // Update display with new measurements
    port_update_display(port);
}

void port_update_leds(Port* port) {
    if (!port || !port->leds) {
        return;
    }
    
    // Update LED states based on port state
    switch (port->state) {
        case PORT_STATE_DISABLED:
            port_leds_set_status(port->leds, false);
            port_leds_set_signal_detected(port->leds, false);
            break;
            
        case PORT_STATE_ENABLED_NO_SIGNAL:
            port_leds_set_status(port->leds, true);
            port_leds_set_signal_detected(port->leds, false);
            break;
            
        case PORT_STATE_ENABLED_CALCULATING:
            port_leds_set_status(port->leds, true);
            // Blink signal LED during calculation
            if (port->leds->signal_led) {
                led_set_state(port->leds->signal_led, LED_STATE_BLINK_FAST);
            }
            break;
            
        case PORT_STATE_ENABLED_LOCKED:
            port_leds_set_status(port->leds, true);
            port_leds_set_signal_detected(port->leds, true);
            break;
    }
}

void port_set_status_led(Port* port, bool on) {
    if (!port || !port->leds || !port->leds->status_led) {
        return;
    }
    
    led_set_state(port->leds->status_led, on ? LED_STATE_ON : LED_STATE_OFF);
}

void port_set_signal_led(Port* port, bool on) {
    if (!port || !port->leds || !port->leds->signal_led) {
        return;
    }
    
    led_set_state(port->leds->signal_led, on ? LED_STATE_ON : LED_STATE_OFF);
}

void port_update_display(Port* port) {
    if (!port || !port->display || !port->enabled) {
        return;
    }
    
    uint32_t current_tick = HAL_GetTick();
    char line1[21], line2[21];
    
    // Toggle between frequency and SNR display every few seconds
    if (current_tick - port->display_toggle_tick > DISPLAY_TOGGLE_INTERVAL_MS) {
        port->display_showing_frequency = !port->display_showing_frequency;
        port->display_toggle_tick = current_tick;
    }
    
    // Format display content based on port state
    switch (port->state) {
        case PORT_STATE_DISABLED:
            // Display should be cleared when disabled
            break;
            
        case PORT_STATE_ENABLED_NO_SIGNAL:
            snprintf(line1, sizeof(line1), "Port %d", port->port_id + 1);
            snprintf(line2, sizeof(line2), "No Signal");
            display_show_two_lines(port->display, line1, line2);
            break;
            
        case PORT_STATE_ENABLED_CALCULATING:
            snprintf(line1, sizeof(line1), "Port %d", port->port_id + 1);
            snprintf(line2, sizeof(line2), "Calculating...");
            display_show_two_lines(port->display, line1, line2);
            break;
            
        case PORT_STATE_ENABLED_LOCKED:
            if (port->display_showing_frequency) {
                snprintf(line1, sizeof(line1), "Port %d", port->port_id + 1);
                snprintf(line2, sizeof(line2), "%.1f MHz", port->frequency_mhz);
            } else {
                snprintf(line1, sizeof(line1), "SNR: %.1f dB", port->snr_db);
                snprintf(line2, sizeof(line2), "%.1f MHz", port->frequency_mhz);
            }
            display_show_two_lines(port->display, line1, line2);
            break;
    }
}

void port_clear_display(Port* port) {
    if (!port || !port->display) {
        return;
    }
    
    display_clear_screen(port->display);
}