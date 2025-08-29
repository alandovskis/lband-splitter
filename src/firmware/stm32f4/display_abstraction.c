#include "display_abstraction.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Private constants
#define MAX_DISPLAYS 32
#define I2C_TIMEOUT_MS 100
#define DISPLAY_UPDATE_INTERVAL_MS 100

// SSD1306 OLED Commands
#define SSD1306_SETCONTRAST 0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF
#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA
#define SSD1306_SETVCOMDETECT 0xDB
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9
#define SSD1306_SETMULTIPLEX 0xA8
#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10
#define SSD1306_SETSTARTLINE 0x40
#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR 0x22
#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8
#define SSD1306_SEGREMAP 0xA0
#define SSD1306_CHARGEPUMP 0x8D

// Global display registry
static DisplayAbstraction* g_displays[MAX_DISPLAYS] = {0};
static uint8_t g_display_count = 0;
static bool g_abstraction_initialized = false;

// Forward declarations of driver implementations
static bool ssd1306_init(DisplayAbstraction* display);
static bool ssd1306_clear(DisplayAbstraction* display);
static bool ssd1306_write_string(DisplayAbstraction* display, const char* str);
static bool ssd1306_write_line(DisplayAbstraction* display, uint8_t line, const char* str);
static bool ssd1306_set_cursor(DisplayAbstraction* display, uint8_t x, uint8_t y);
static bool ssd1306_power_on(DisplayAbstraction* display);
static bool ssd1306_power_off(DisplayAbstraction* display);
static bool ssd1306_is_ready(DisplayAbstraction* display);

// I2C utility functions
static HAL_StatusTypeDef i2c_write_command(I2C_HandleTypeDef* hi2c, uint8_t addr, uint8_t cmd);
static HAL_StatusTypeDef i2c_write_data(I2C_HandleTypeDef* hi2c, uint8_t addr, uint8_t* data, uint16_t len);

// Global display management functions
bool display_abstraction_init(void) {
    if (g_abstraction_initialized) {
        return true;
    }
    
    // Clear display registry
    memset(g_displays, 0, sizeof(g_displays));
    g_display_count = 0;
    g_abstraction_initialized = true;
    
    return true;
}

DisplayAbstraction* display_create(uint8_t port_id, DisplayType type, 
                                 uint8_t i2c_addr, I2C_HandleTypeDef* hi2c) {
    if (!g_abstraction_initialized || g_display_count >= MAX_DISPLAYS) {
        return NULL;
    }
    
    DisplayAbstraction* display = (DisplayAbstraction*)calloc(1, sizeof(DisplayAbstraction));
    if (!display) {
        return NULL;
    }
    
    // Configure hardware
    display->config.port_id = port_id;
    display->config.type = type;
    display->config.i2c_address = i2c_addr;
    display->config.mux_address = 0;
    display->config.mux_channel = 0;
    display->config.use_multiplexer = false;
    display->config.hi2c = hi2c;
    display->config.initialized = false;
    display->config.enabled = false;
    display->config.last_update_ms = 0;
    
    // Set display size based on type
    switch (type) {
        case DISPLAY_TYPE_OLED_SSD1306:
            display->config.size.width = 128;
            display->config.size.height = 64;
            display->config.size.pixel_based = true;
            break;
        case DISPLAY_TYPE_LCD_HD44780:
            display->config.size.width = 20;
            display->config.size.height = 4;
            display->config.size.pixel_based = false;
            break;
        case DISPLAY_TYPE_SEVEN_SEGMENT:
            display->config.size.width = 8;
            display->config.size.height = 1;
            display->config.size.pixel_based = false;
            break;
        default:
            free(display);
            return NULL;
    }
    
    // Initialize content
    memset(&display->content, 0, sizeof(DisplayContent));
    display->content.backlight_on = true;
    
    // Set function pointers based on display type
    switch (type) {
        case DISPLAY_TYPE_OLED_SSD1306:
            display->init = ssd1306_init;
            display->clear = ssd1306_clear;
            display->set_cursor = ssd1306_set_cursor;
            display->write_string = ssd1306_write_string;
            display->write_line = ssd1306_write_line;
            display->power_on = ssd1306_power_on;
            display->power_off = ssd1306_power_off;
            display->is_ready = ssd1306_is_ready;
            display->set_backlight = NULL; // OLED doesn't have backlight
            break;
        default:
            free(display);
            return NULL;
    }
    
    // Register display
    g_displays[g_display_count++] = display;
    
    return display;
}

DisplayAbstraction* display_create_multiplexed(uint8_t port_id, DisplayType type,
                                             uint8_t i2c_addr, uint8_t mux_addr, 
                                             uint8_t mux_channel, I2C_HandleTypeDef* hi2c) {
    DisplayAbstraction* display = display_create(port_id, type, i2c_addr, hi2c);
    if (!display) {
        return NULL;
    }
    
    // Configure multiplexer
    display->config.use_multiplexer = true;
    display->config.mux_address = mux_addr;
    display->config.mux_channel = mux_channel;
    
    return display;
}

bool display_destroy(DisplayAbstraction* display) {
    if (!display) {
        return false;
    }
    
    // Remove from registry
    for (uint8_t i = 0; i < g_display_count; i++) {
        if (g_displays[i] == display) {
            // Shift remaining displays
            for (uint8_t j = i; j < g_display_count - 1; j++) {
                g_displays[j] = g_displays[j + 1];
            }
            g_displays[--g_display_count] = NULL;
            break;
        }
    }
    
    // Power off and free
    if (display->power_off) {
        display->power_off(display);
    }
    free(display);
    
    return true;
}

// Common display operations
bool display_initialize(DisplayAbstraction* display) {
    if (!display || !display->init) {
        return false;
    }
    
    return display->init(display);
}

bool display_clear_screen(DisplayAbstraction* display) {
    if (!display || !display->clear || !display->config.initialized) {
        return false;
    }
    
    return display->clear(display);
}

bool display_show_text(DisplayAbstraction* display, const char* text) {
    if (!display || !display->write_string || !display->config.initialized || !text) {
        return false;
    }
    
    return display->write_string(display, text);
}

bool display_show_frequency(DisplayAbstraction* display, double frequency_mhz) {
    if (!display) {
        return false;
    }
    
    char freq_str[21];
    snprintf(freq_str, sizeof(freq_str), "%.1f MHz", frequency_mhz);
    
    return display_show_text(display, freq_str);
}

bool display_show_two_lines(DisplayAbstraction* display, const char* line1, const char* line2) {
    if (!display || !display->write_line || !display->config.initialized) {
        return false;
    }
    
    bool result = true;
    if (line1) {
        result &= display->write_line(display, 0, line1);
    }
    if (line2) {
        result &= display->write_line(display, 1, line2);
    }
    
    return result;
}

bool display_set_enabled(DisplayAbstraction* display, bool enabled) {
    if (!display) {
        return false;
    }
    
    display->config.enabled = enabled;
    
    if (enabled && display->power_on) {
        return display->power_on(display);
    } else if (!enabled && display->power_off) {
        return display->power_off(display);
    }
    
    return true;
}

bool display_update(DisplayAbstraction* display) {
    if (!display || !display->config.initialized) {
        return false;
    }
    
    uint32_t current_ms = HAL_GetTick();
    if (current_ms - display->config.last_update_ms < DISPLAY_UPDATE_INTERVAL_MS) {
        return true; // Skip update if too soon
    }
    
    display->config.last_update_ms = current_ms;
    return true;
}

// Utility functions
bool display_select_mux_channel(I2C_HandleTypeDef* hi2c, uint8_t mux_addr, uint8_t channel) {
    if (!hi2c || channel > 7) {
        return false;
    }
    
    uint8_t channel_byte = 1 << channel;
    HAL_StatusTypeDef result = HAL_I2C_Master_Transmit(hi2c, mux_addr << 1, &channel_byte, 1, I2C_TIMEOUT_MS);
    return result == HAL_OK;
}

const char* display_type_to_string(DisplayType type) {
    switch (type) {
        case DISPLAY_TYPE_OLED_SSD1306: return "SSD1306 OLED";
        case DISPLAY_TYPE_LCD_HD44780: return "HD44780 LCD";
        case DISPLAY_TYPE_SEVEN_SEGMENT: return "7-Segment";
        case DISPLAY_TYPE_NONE: return "None";
        default: return "Unknown";
    }
}

// SSD1306 OLED driver implementation
static bool ssd1306_init(DisplayAbstraction* display) {
    if (!display || !display->config.hi2c) {
        return false;
    }
    
    // Select multiplexer channel if needed
    if (display->config.use_multiplexer) {
        if (!display_select_mux_channel(display->config.hi2c, 
                                      display->config.mux_address, 
                                      display->config.mux_channel)) {
            return false;
        }
    }
    
    // SSD1306 initialization sequence
    uint8_t init_commands[] = {
        SSD1306_DISPLAYOFF,
        SSD1306_SETDISPLAYCLOCKDIV, 0x80,
        SSD1306_SETMULTIPLEX, 0x3F,
        SSD1306_SETDISPLAYOFFSET, 0x00,
        SSD1306_SETSTARTLINE | 0x00,
        SSD1306_CHARGEPUMP, 0x14,
        SSD1306_MEMORYMODE, 0x00,
        SSD1306_SEGREMAP | 0x01,
        SSD1306_COMSCANDEC,
        SSD1306_SETCOMPINS, 0x12,
        SSD1306_SETCONTRAST, 0xCF,
        SSD1306_SETPRECHARGE, 0xF1,
        SSD1306_SETVCOMDETECT, 0x40,
        SSD1306_DISPLAYALLON_RESUME,
        SSD1306_NORMALDISPLAY,
        SSD1306_DISPLAYON
    };
    
    for (size_t i = 0; i < sizeof(init_commands); i++) {
        if (i2c_write_command(display->config.hi2c, display->config.i2c_address, init_commands[i]) != HAL_OK) {
            return false;
        }
    }
    
    display->config.initialized = true;
    display->config.enabled = true;
    
    // Clear display
    return ssd1306_clear(display);
}

static bool ssd1306_clear(DisplayAbstraction* display) {
    if (!display || !display->config.initialized) {
        return false;
    }
    
    // Select multiplexer channel if needed
    if (display->config.use_multiplexer) {
        if (!display_select_mux_channel(display->config.hi2c, 
                                      display->config.mux_address, 
                                      display->config.mux_channel)) {
            return false;
        }
    }
    
    // Set column and page address
    if (i2c_write_command(display->config.hi2c, display->config.i2c_address, SSD1306_COLUMNADDR) != HAL_OK) return false;
    if (i2c_write_command(display->config.hi2c, display->config.i2c_address, 0) != HAL_OK) return false;
    if (i2c_write_command(display->config.hi2c, display->config.i2c_address, 127) != HAL_OK) return false;
    
    if (i2c_write_command(display->config.hi2c, display->config.i2c_address, SSD1306_PAGEADDR) != HAL_OK) return false;
    if (i2c_write_command(display->config.hi2c, display->config.i2c_address, 0) != HAL_OK) return false;
    if (i2c_write_command(display->config.hi2c, display->config.i2c_address, 7) != HAL_OK) return false;
    
    // Clear display memory (128x64 / 8 = 1024 bytes)
    uint8_t clear_data[128] = {0};
    for (int page = 0; page < 8; page++) {
        if (i2c_write_data(display->config.hi2c, display->config.i2c_address, clear_data, 128) != HAL_OK) {
            return false;
        }
    }
    
    // Clear content structure
    memset(&display->content, 0, sizeof(DisplayContent));
    display->content.backlight_on = true;
    
    return true;
}

static bool ssd1306_write_string(DisplayAbstraction* display, const char* str) {
    if (!display || !str) {
        return false;
    }
    
    // For simplicity, just store in line1 and use write_line
    strncpy(display->content.line1, str, sizeof(display->content.line1) - 1);
    display->content.line1[sizeof(display->content.line1) - 1] = '\0';
    
    return ssd1306_write_line(display, 0, display->content.line1);
}

static bool ssd1306_write_line(DisplayAbstraction* display, uint8_t line, const char* str) {
    if (!display || !str || line >= 2 || !display->config.initialized) {
        return false;
    }
    
    // Select multiplexer channel if needed
    if (display->config.use_multiplexer) {
        if (!display_select_mux_channel(display->config.hi2c, 
                                      display->config.mux_address, 
                                      display->config.mux_channel)) {
            return false;
        }
    }
    
    // Simple text implementation - for a full implementation, you'd need font data
    // This is a placeholder that just shows the concept
    
    // Store in content structure
    if (line == 0) {
        strncpy(display->content.line1, str, sizeof(display->content.line1) - 1);
        display->content.line1[sizeof(display->content.line1) - 1] = '\0';
    } else {
        strncpy(display->content.line2, str, sizeof(display->content.line2) - 1);
        display->content.line2[sizeof(display->content.line2) - 1] = '\0';
    }
    
    return true;
}

static bool ssd1306_set_cursor(DisplayAbstraction* display, uint8_t x, uint8_t y) {
    if (!display) {
        return false;
    }
    
    display->content.cursor_x = x;
    display->content.cursor_y = y;
    return true;
}

static bool ssd1306_power_on(DisplayAbstraction* display) {
    if (!display || !display->config.hi2c) {
        return false;
    }
    
    if (display->config.use_multiplexer) {
        if (!display_select_mux_channel(display->config.hi2c, 
                                      display->config.mux_address, 
                                      display->config.mux_channel)) {
            return false;
        }
    }
    
    return i2c_write_command(display->config.hi2c, display->config.i2c_address, SSD1306_DISPLAYON) == HAL_OK;
}

static bool ssd1306_power_off(DisplayAbstraction* display) {
    if (!display || !display->config.hi2c) {
        return false;
    }
    
    if (display->config.use_multiplexer) {
        if (!display_select_mux_channel(display->config.hi2c, 
                                      display->config.mux_address, 
                                      display->config.mux_channel)) {
            return false;
        }
    }
    
    return i2c_write_command(display->config.hi2c, display->config.i2c_address, SSD1306_DISPLAYOFF) == HAL_OK;
}

static bool ssd1306_is_ready(DisplayAbstraction* display) {
    if (!display || !display->config.hi2c) {
        return false;
    }
    
    if (display->config.use_multiplexer) {
        if (!display_select_mux_channel(display->config.hi2c, 
                                      display->config.mux_address, 
                                      display->config.mux_channel)) {
            return false;
        }
    }
    
    // Test I2C communication
    return HAL_I2C_IsDeviceReady(display->config.hi2c, 
                                display->config.i2c_address << 1, 
                                3, I2C_TIMEOUT_MS) == HAL_OK;
}

// I2C utility functions
static HAL_StatusTypeDef i2c_write_command(I2C_HandleTypeDef* hi2c, uint8_t addr, uint8_t cmd) {
    uint8_t data[2] = {0x00, cmd}; // 0x00 = command mode for SSD1306
    return HAL_I2C_Master_Transmit(hi2c, addr << 1, data, 2, I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef i2c_write_data(I2C_HandleTypeDef* hi2c, uint8_t addr, uint8_t* data, uint16_t len) {
    // For SSD1306, prepend 0x40 to indicate data mode
    uint8_t* buffer = malloc(len + 1);
    if (!buffer) {
        return HAL_ERROR;
    }
    
    buffer[0] = 0x40; // Data mode
    memcpy(&buffer[1], data, len);
    
    HAL_StatusTypeDef result = HAL_I2C_Master_Transmit(hi2c, addr << 1, buffer, len + 1, I2C_TIMEOUT_MS);
    free(buffer);
    
    return result;
}