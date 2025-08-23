#pragma once
#include <cstdint>

typedef struct {
    int dummy;
} UART_HandleTypeDef;
 
#ifdef __cplusplus
extern "C" {
#endif

void HAL_UART_Transmit(UART_HandleTypeDef*, uint8_t*, uint16_t, uint32_t);

#ifndef HAL_MAX_DELAY
#define HAL_MAX_DELAY 0xFFFFFFFFU
#endif

#ifdef __cplusplus
}
#endif
