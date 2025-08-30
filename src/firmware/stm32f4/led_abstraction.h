#ifndef LED_ABSTRACTION_H
#define LED_ABSTRACTION_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// LED types and behaviors
typedef enum {
  LED_TYPE_STATUS = 0, // Status LED (enabled/disabled)
  LED_TYPE_SIGNAL,     // Signal detection LED
  LED_TYPE_ERROR,      // Error indication LED
  LED_TYPE_ACTIVITY    // Activity/processing LED
} LedType;

typedef enum {
  LED_STATE_OFF = 0,
  LED_STATE_ON,
  LED_STATE_BLINK_SLOW, // 1 Hz blink
  LED_STATE_BLINK_FAST, // 4 Hz blink
  LED_STATE_PULSE,      // Breathing effect
  LED_STATE_FLASH       // Single flash
} LedState;

typedef enum {
  LED_COLOR_RED = 0,
  LED_COLOR_GREEN,
  LED_COLOR_BLUE,
  LED_COLOR_YELLOW,
  LED_COLOR_WHITE,
  LED_COLOR_RGB // Multi-color LED
} LedColor;

// LED hardware configuration
typedef struct {
  uint8_t led_id;  // Unique LED identifier
  uint8_t port_id; // Port this LED belongs to
  LedType type;    // LED purpose/type
  LedColor color;  // LED color

  // GPIO configuration
  GPIO_TypeDef *gpio_port; // GPIO port (GPIOA, GPIOB, etc.)
  uint16_t gpio_pin;       // GPIO pin (GPIO_PIN_0, etc.)
  bool active_high;        // true = high turns on, false = low turns on

  // PWM configuration (for brightness/pulse effects)
  TIM_HandleTypeDef *htim; // Timer handle for PWM (optional)
  uint32_t tim_channel;    // Timer channel for PWM
  bool pwm_enabled;        // Whether PWM is available

  // RGB configuration (for multi-color LEDs)
  struct {
    GPIO_TypeDef *red_port;
    uint16_t red_pin;
    GPIO_TypeDef *green_port;
    uint16_t green_pin;
    GPIO_TypeDef *blue_port;
    uint16_t blue_pin;
    TIM_HandleTypeDef *red_timer;
    uint32_t red_channel;
    TIM_HandleTypeDef *green_timer;
    uint32_t green_channel;
    TIM_HandleTypeDef *blue_timer;
    uint32_t blue_channel;
  } rgb;

  // State
  bool initialized;        // LED initialization status
  LedState current_state;  // Current LED state
  uint8_t brightness;      // Brightness level (0-100)
  uint32_t last_update_ms; // Last state update timestamp

} LedConfig;

// LED timing configuration for different blink patterns
typedef struct {
  uint16_t slow_blink_period_ms; // Slow blink period (default: 1000ms)
  uint16_t fast_blink_period_ms; // Fast blink period (default: 250ms)
  uint16_t pulse_period_ms;      // Pulse breathing period (default: 2000ms)
  uint16_t flash_duration_ms;    // Flash duration (default: 100ms)
  uint8_t pulse_min_brightness;  // Minimum brightness for pulse (0-100)
  uint8_t pulse_max_brightness;  // Maximum brightness for pulse (0-100)
} LedTimingConfig;

// LED abstraction interface
typedef struct {
  // Hardware configuration
  LedConfig config;
  LedTimingConfig timing;

  // Internal state for pattern generation
  struct {
    bool pattern_active;        // Whether pattern is currently active
    uint32_t pattern_start_ms;  // When current pattern started
    uint32_t last_toggle_ms;    // Last toggle time for blink patterns
    bool toggle_state;          // Current toggle state for blinking
    uint8_t current_brightness; // Current brightness for PWM/pulse
    uint32_t flash_count;       // Number of flashes remaining
  } state;

  // Function pointers for hardware-specific implementations
  bool (*init)(struct LedAbstraction *led);
  bool (*set_on)(struct LedAbstraction *led);
  bool (*set_off)(struct LedAbstraction *led);
  bool (*set_brightness)(struct LedAbstraction *led, uint8_t brightness);
  bool (*set_rgb_color)(struct LedAbstraction *led, uint8_t r, uint8_t g,
                        uint8_t b);
  bool (*toggle)(struct LedAbstraction *led);
  bool (*is_on)(struct LedAbstraction *led);

  // Private data for driver implementations
  void *driver_data;

} LedAbstraction;

// Global LED management functions
bool led_abstraction_init(void);
LedAbstraction *led_create(uint8_t led_id, uint8_t port_id, LedType type,
                           LedColor color);
LedAbstraction *led_create_simple(uint8_t led_id, uint8_t port_id, LedType type,
                                  GPIO_TypeDef *gpio_port, uint16_t gpio_pin,
                                  bool active_high);
LedAbstraction *led_create_pwm(uint8_t led_id, uint8_t port_id, LedType type,
                               GPIO_TypeDef *gpio_port, uint16_t gpio_pin,
                               bool active_high, TIM_HandleTypeDef *htim,
                               uint32_t channel);
LedAbstraction *led_create_rgb(uint8_t led_id, uint8_t port_id,
                               GPIO_TypeDef *r_port, uint16_t r_pin,
                               GPIO_TypeDef *g_port, uint16_t g_pin,
                               GPIO_TypeDef *b_port, uint16_t b_pin);
bool led_destroy(LedAbstraction *led);

// LED control functions
bool led_initialize(LedAbstraction *led);
bool led_set_state(LedAbstraction *led, LedState state);
bool led_set_brightness(LedAbstraction *led, uint8_t brightness);
bool led_set_color(LedAbstraction *led, uint8_t r, uint8_t g, uint8_t b);
bool led_flash_once(LedAbstraction *led);
bool led_flash_count(LedAbstraction *led, uint32_t count);
bool led_update(LedAbstraction *led); // Call this regularly to update patterns
LedState led_get_state(LedAbstraction *led);
bool led_is_active(LedAbstraction *led);

// LED pattern control
bool led_start_pattern(LedAbstraction *led, LedState pattern);
bool led_stop_pattern(LedAbstraction *led);
bool led_configure_timing(LedAbstraction *led, const LedTimingConfig *timing);

// Utility functions
const char *led_state_to_string(LedState state);
const char *led_type_to_string(LedType type);
const char *led_color_to_string(LedColor color);

// Port-specific LED management
typedef struct {
  uint8_t port_id;
  LedAbstraction *status_led;   // Port enable/disable status
  LedAbstraction *signal_led;   // Signal detection status
  LedAbstraction *error_led;    // Error indication (optional)
  LedAbstraction *activity_led; // Processing activity (optional)
} PortLedGroup;

PortLedGroup *port_leds_create(uint8_t port_id);
bool port_leds_destroy(PortLedGroup *group);
bool port_leds_set_status(PortLedGroup *group, bool enabled);
bool port_leds_set_signal_detected(PortLedGroup *group, bool detected);
bool port_leds_set_calculating(PortLedGroup *group, bool calculating);
bool port_leds_set_error(PortLedGroup *group, bool error);
bool port_leds_update_all(PortLedGroup *group);

#ifdef __cplusplus
}
#endif

#endif // LED_ABSTRACTION_H