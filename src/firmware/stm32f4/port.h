#ifndef PORT_H
#define PORT_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PORT_STATE_DISABLED = 0,
    PORT_STATE_ENABLED_NO_SIGNAL,
    PORT_STATE_ENABLED_CALCULATING,
    PORT_STATE_ENABLED_LOCKED
} PortState;

typedef struct {
    uint8_t port_id;
    PortState state;
    bool enabled;
    bool signal_detected;
    bool calculation_active;
    
    // Measurements
    double frequency_mhz;
    double snr_db;
    uint32_t last_update_tick;
    
    // Display configuration
    uint8_t display_i2c_address;
    uint8_t display_mux_channel;
    bool display_showing_frequency;
    uint32_t display_toggle_tick;
    
    // LED GPIO configuration
    GPIO_TypeDef* status_led_port;
    uint16_t status_led_pin;
    GPIO_TypeDef* signal_led_port;
    uint16_t signal_led_pin;
    uint32_t signal_led_blink_tick;
} Port;

// Port management functions
void port_init(Port* port, uint8_t port_id);
void port_update(Port* port);
void port_set_enabled(Port* port, bool enabled);
void port_set_signal_detection(Port* port, bool detected);
void port_set_calculation_active(Port* port, bool active);
void port_update_measurements(Port* port, double frequency_mhz, double snr_db);

// LED control functions
void port_update_leds(Port* port);
void port_set_status_led(Port* port, bool on);
void port_set_signal_led(Port* port, bool on);

// Display control functions
void port_update_display(Port* port);
void port_clear_display(Port* port);

#ifdef __cplusplus
}
#endif

#endif // PORT_H