#ifndef PORT_H
#define PORT_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct DisplayAbstraction DisplayAbstraction;
typedef struct PortLedGroup PortLedGroup;

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
    
    // Hardware abstractions
    DisplayAbstraction* display;
    PortLedGroup* leds;
    
    // Display state
    bool display_showing_frequency;
    uint32_t display_toggle_tick;
} Port;

// Port management functions
void port_init(Port* port, uint8_t port_id);
void port_update(Port* port);
void port_set_enabled(Port* port, bool enabled);
void port_set_signal_detection(Port* port, bool detected);
void port_set_calculation_active(Port* port, bool active);
void port_update_measurements(Port* port, double frequency_mhz, double snr_db);

// Hardware abstraction functions
bool port_init_hardware(Port* port, I2C_HandleTypeDef* hi2c);
void port_cleanup_hardware(Port* port);

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