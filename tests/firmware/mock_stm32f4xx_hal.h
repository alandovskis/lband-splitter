#ifndef MOCK_STM32F4XX_HAL_H
#define MOCK_STM32F4XX_HAL_H

#include <stdbool.h>
#include <stdint.h>

// Mock GPIO definitions
typedef struct {
  uint32_t dummy;
} GPIO_TypeDef;

typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET } GPIO_PinState;

// Mock GPIO pins
#define GPIO_PIN_0 (0x0001)
#define GPIO_PIN_1 (0x0002)
#define GPIO_PIN_2 (0x0004)
#define GPIO_PIN_3 (0x0008)
#define GPIO_PIN_4 (0x0010)
#define GPIO_PIN_5 (0x0020)
#define GPIO_PIN_6 (0x0040)
#define GPIO_PIN_7 (0x0080)
#define GPIO_PIN_8 (0x0100)
#define GPIO_PIN_9 (0x0200)
#define GPIO_PIN_10 (0x0400)
#define GPIO_PIN_11 (0x0800)
#define GPIO_PIN_12 (0x1000)
#define GPIO_PIN_13 (0x2000)
#define GPIO_PIN_14 (0x4000)
#define GPIO_PIN_15 (0x8000)

// Mock GPIO ports
extern GPIO_TypeDef GPIOA_MOCK;
extern GPIO_TypeDef GPIOB_MOCK;
extern GPIO_TypeDef GPIOC_MOCK;
extern GPIO_TypeDef GPIOD_MOCK;

#define GPIOA (&GPIOA_MOCK)
#define GPIOB (&GPIOB_MOCK)
#define GPIOC (&GPIOC_MOCK)
#define GPIOD (&GPIOD_MOCK)

// Mock function declarations
uint32_t HAL_GetTick(void);
void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin,
                       GPIO_PinState PinState);

#endif // MOCK_STM32F4XX_HAL_H