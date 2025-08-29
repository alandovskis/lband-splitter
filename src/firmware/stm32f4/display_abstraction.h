#ifndef DISPLAY_ABSTRACTION_H
#define DISPLAY_ABSTRACTION_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Display types
typedef enum {
    DISPLAY_TYPE_OLED_SSD1306 = 0,
    DISPLAY_TYPE_LCD_HD44780,
    DISPLAY_TYPE_SEVEN_SEGMENT,
    DISPLAY_TYPE_NONE
} DisplayType;

// Display size configuration
typedef struct {
    uint8_t width;    // Characters or pixels
    uint8_t height;   // Characters or pixels
    bool pixel_based; // true = pixel coordinates, false = character coordinates
} DisplaySize;

// Display hardware configuration
typedef struct {
    uint8_t port_id;                    // Port this display belongs to
    DisplayType type;                   // Type of display
    DisplaySize size;                   // Display dimensions
    
    // I2C configuration
    uint8_t i2c_address;               // Direct I2C address or multiplexed address
    uint8_t mux_address;               // I2C multiplexer address (0 if none)
    uint8_t mux_channel;               // Multiplexer channel (0-7)
    bool use_multiplexer;              // Whether to use I2C multiplexer
    
    // Hardware handles
    I2C_HandleTypeDef* hi2c;           // I2C handle
    
    // State
    bool initialized;                   // Display initialization status
    bool enabled;                      // Display power state
    uint32_t last_update_ms;           // Last update timestamp
} DisplayConfig;

// Display content structure
typedef struct {
    char line1[21];  // First line of text (up to 20 chars + null)
    char line2[21];  // Second line of text (up to 20 chars + null)
    uint8_t cursor_x;
    uint8_t cursor_y;
    bool cursor_visible;
    bool backlight_on;
} DisplayContent;

// Display abstraction interface
typedef struct {
    // Hardware configuration
    DisplayConfig config;
    DisplayContent content;
    
    // Function pointers for hardware-specific implementations
    bool (*init)(struct DisplayAbstraction* display);
    bool (*clear)(struct DisplayAbstraction* display);
    bool (*set_cursor)(struct DisplayAbstraction* display, uint8_t x, uint8_t y);
    bool (*write_char)(struct DisplayAbstraction* display, char c);
    bool (*write_string)(struct DisplayAbstraction* display, const char* str);
    bool (*write_line)(struct DisplayAbstraction* display, uint8_t line, const char* str);
    bool (*set_backlight)(struct DisplayAbstraction* display, bool on);
    bool (*power_on)(struct DisplayAbstraction* display);
    bool (*power_off)(struct DisplayAbstraction* display);
    bool (*is_ready)(struct DisplayAbstraction* display);
    
    // Private data for driver implementations
    void* driver_data;
    
} DisplayAbstraction;

// Global display management functions
bool display_abstraction_init(void);
DisplayAbstraction* display_create(uint8_t port_id, DisplayType type, 
                                 uint8_t i2c_addr, I2C_HandleTypeDef* hi2c);
DisplayAbstraction* display_create_multiplexed(uint8_t port_id, DisplayType type,
                                             uint8_t i2c_addr, uint8_t mux_addr, 
                                             uint8_t mux_channel, I2C_HandleTypeDef* hi2c);
bool display_destroy(DisplayAbstraction* display);

// Common display operations
bool display_initialize(DisplayAbstraction* display);
bool display_clear_screen(DisplayAbstraction* display);
bool display_show_text(DisplayAbstraction* display, const char* text);
bool display_show_frequency(DisplayAbstraction* display, double frequency_mhz);
bool display_show_two_lines(DisplayAbstraction* display, const char* line1, const char* line2);
bool display_set_enabled(DisplayAbstraction* display, bool enabled);
bool display_update(DisplayAbstraction* display);

// Utility functions
bool display_select_mux_channel(I2C_HandleTypeDef* hi2c, uint8_t mux_addr, uint8_t channel);
const char* display_type_to_string(DisplayType type);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_ABSTRACTION_H