#include "port.h"
#include "display.h"
#include <stdio.h>

// Constants
#define DISPLAY_TOGGLE_INTERVAL_MS 2000
#define SIGNAL_LED_BLINK_INTERVAL_MS 500

// GPIO pin mappings for LEDs (A0-A15 for status, B0-B15 for signal, C0-C15 for status overflow)
static const GPIO_TypeDef* STATUS_LED_PORTS[32] = {
    GPIOA, GPIOA, GPIOA, GPIOA, GPIOA, GPIOA, GPIOA, GPIOA,
    GPIOA, GPIOA, GPIOA, GPIOA, GPIOA, GPIOA, GPIOA, GPIOA,
    GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC,
    GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC, GPIOC
};

static const uint16_t STATUS_LED_PINS[32] = {
    GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7,
    GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11, GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15,
    GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7,
    GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11, GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15
};

static const GPIO_TypeDef* SIGNAL_LED_PORTS[32] = {
    GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB,
    GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB, GPIOB,
    GPIOD, GPIOD, GPIOD, GPIOD, GPIOD, GPIOD, GPIOD, GPIOD,
    GPIOD, GPIOD, GPIOD, GPIOD, GPIOD, GPIOD, GPIOD, GPIOD
};

static const uint16_t SIGNAL_LED_PINS[32] = {
    GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7,
    GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11, GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15,
    GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3, GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7,
    GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11, GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15
};

void port_init(Port* port, uint8_t port_id) {
    port->port_id = port_id;
    port->state = PORT_STATE_DISABLED;
    port->enabled = false;
    port->signal_detected = false;
    port->calculation_active = false;
    
    port->frequency_mhz = 0.0;
    port->snr_db = 0.0;
    port->last_update_tick = 0;
    
    // Display configuration
    port->display_i2c_address = 0x3C;
    port->display_mux_channel = port_id % 8;
    port->display_showing_frequency = true;
    port->display_toggle_tick = 0;
    
    // LED GPIO configuration
    port->status_led_port = (GPIO_TypeDef*)STATUS_LED_PORTS[port_id];
    port->status_led_pin = STATUS_LED_PINS[port_id];
    port->signal_led_port = (GPIO_TypeDef*)SIGNAL_LED_PORTS[port_id];
    port->signal_led_pin = SIGNAL_LED_PINS[port_id];
    port->signal_led_blink_tick = 0;
    
    // Initialize LEDs to off
    port_set_status_led(port, false);
    port_set_signal_led(port, false);
    
    // Clear display
    port_clear_display(port);
}

void port_update(Port* port) {
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
}

void port_set_enabled(Port* port, bool enabled) {
    port->enabled = enabled;
    if (!enabled) {
        port->signal_detected = false;
        port->calculation_active = false;
    }
}

void port_set_signal_detection(Port* port, bool detected) {
    port->signal_detected = detected;
    if (!detected) {
        port->calculation_active = false;
    }
}

void port_set_calculation_active(Port* port, bool active) {
    port->calculation_active = active;
}

void port_update_measurements(Port* port, double frequency_mhz, double snr_db) {
    port->frequency_mhz = frequency_mhz;
    port->snr_db = snr_db;
    port->last_update_tick = HAL_GetTick();
}

void port_update_leds(Port* port) {
    uint32_t current_tick = HAL_GetTick();
    
    // Status LED: on when enabled, off when disabled
    port_set_status_led(port, port->enabled);
    
    // Signal LED behavior based on state
    switch (port->state) {
        case PORT_STATE_DISABLED:
        case PORT_STATE_ENABLED_NO_SIGNAL:
            port_set_signal_led(port, false);
            break;
            
        case PORT_STATE_ENABLED_CALCULATING:
            // Blink signal LED during calculation
            if (current_tick - port->signal_led_blink_tick >= SIGNAL_LED_BLINK_INTERVAL_MS) {
                static bool blink_state = false;
                blink_state = !blink_state;
                port_set_signal_led(port, blink_state);
                port->signal_led_blink_tick = current_tick;
            }
            break;
            
        case PORT_STATE_ENABLED_LOCKED:
            port_set_signal_led(port, true);
            break;
    }
}

void port_set_status_led(Port* port, bool on) {
    HAL_GPIO_WritePin(port->status_led_port, port->status_led_pin, 
                      on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void port_set_signal_led(Port* port, bool on) {
    HAL_GPIO_WritePin(port->signal_led_port, port->signal_led_pin, 
                      on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void port_update_display(Port* port) {
    uint32_t current_tick = HAL_GetTick();
    
    if (port->state != PORT_STATE_ENABLED_LOCKED) {
        // Display nothing if not locked
        port_clear_display(port);
        return;
    }
    
    // Toggle between frequency and SNR every 2 seconds
    if (current_tick - port->display_toggle_tick >= DISPLAY_TOGGLE_INTERVAL_MS) {
        port->display_showing_frequency = !port->display_showing_frequency;
        port->display_toggle_tick = current_tick;
    }
    
    char display_text[32];
    if (port->display_showing_frequency) {
        snprintf(display_text, sizeof(display_text), "F: %.2f MHz", port->frequency_mhz);
    } else {
        snprintf(display_text, sizeof(display_text), "SNR: %.1f dB", port->snr_db);
    }
    
    // Select the correct I2C mux channel for this port
    uint8_t mux_select = (port->port_id / 8);
    uint8_t mux_channel = port->port_id % 8;
    
    display_select_mux_channel(mux_select, mux_channel);
    display_show_text(port->display_i2c_address, display_text);
}

void port_clear_display(Port* port) {
    // Select the correct I2C mux channel for this port
    uint8_t mux_select = (port->port_id / 8);
    uint8_t mux_channel = port->port_id % 8;
    
    display_select_mux_channel(mux_select, mux_channel);
    display_clear(port->display_i2c_address);
}