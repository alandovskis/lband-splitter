#ifndef DISPLAY_H
#define DISPLAY_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Display initialization and control
void display_init(void);
void display_clear(uint8_t display_address);
void display_show_text(uint8_t display_address, const char *text);

// I2C multiplexer control
void display_select_mux_channel(uint8_t mux_select, uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_H