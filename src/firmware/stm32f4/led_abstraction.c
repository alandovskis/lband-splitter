#include "led_abstraction.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// Private constants
#define MAX_LEDS 128  // Support up to 128 LEDs (32 ports × 4 LEDs each)
#define MAX_PORT_LED_GROUPS 32
#define PWM_MAX_VALUE 1000  // PWM counter period for brightness control

// Default timing configuration
static const LedTimingConfig DEFAULT_TIMING = {
    .slow_blink_period_ms = 1000,
    .fast_blink_period_ms = 250,
    .pulse_period_ms = 2000,
    .flash_duration_ms = 100,
    .pulse_min_brightness = 10,
    .pulse_max_brightness = 100
};

// Global LED registry
static LedAbstraction* g_leds[MAX_LEDS] = {0};
static PortLedGroup* g_port_groups[MAX_PORT_LED_GROUPS] = {0};
static uint8_t g_led_count = 0;
static bool g_abstraction_initialized = false;

// Forward declarations of driver implementations
static bool gpio_led_init(LedAbstraction* led);
static bool gpio_led_set_on(LedAbstraction* led);
static bool gpio_led_set_off(LedAbstraction* led);
static bool gpio_led_toggle(LedAbstraction* led);
static bool gpio_led_is_on(LedAbstraction* led);
static bool pwm_led_set_brightness(LedAbstraction* led, uint8_t brightness);
static bool rgb_led_set_color(LedAbstraction* led, uint8_t r, uint8_t g, uint8_t b);

// Pattern generation functions
static void led_update_blink_pattern(LedAbstraction* led);
static void led_update_pulse_pattern(LedAbstraction* led);
static void led_update_flash_pattern(LedAbstraction* led);

// Global LED management functions
bool led_abstraction_init(void) {
    if (g_abstraction_initialized) {
        return true;
    }
    
    // Clear LED registry
    memset(g_leds, 0, sizeof(g_leds));
    memset(g_port_groups, 0, sizeof(g_port_groups));
    g_led_count = 0;
    g_abstraction_initialized = true;
    
    return true;
}

LedAbstraction* led_create_simple(uint8_t led_id, uint8_t port_id, LedType type,
                                 GPIO_TypeDef* gpio_port, uint16_t gpio_pin, bool active_high) {
    if (!g_abstraction_initialized || g_led_count >= MAX_LEDS) {
        return NULL;
    }
    
    LedAbstraction* led = (LedAbstraction*)calloc(1, sizeof(LedAbstraction));
    if (!led) {
        return NULL;
    }
    
    // Configure hardware
    led->config.led_id = led_id;
    led->config.port_id = port_id;
    led->config.type = type;
    led->config.color = LED_COLOR_RED; // Default for simple LEDs
    led->config.gpio_port = gpio_port;
    led->config.gpio_pin = gpio_pin;
    led->config.active_high = active_high;
    led->config.htim = NULL;
    led->config.tim_channel = 0;
    led->config.pwm_enabled = false;
    led->config.initialized = false;
    led->config.current_state = LED_STATE_OFF;
    led->config.brightness = 100;
    led->config.last_update_ms = 0;
    
    // Initialize timing configuration
    memcpy(&led->timing, &DEFAULT_TIMING, sizeof(LedTimingConfig));
    
    // Initialize state
    memset(&led->state, 0, sizeof(led->state));
    
    // Set function pointers for GPIO LED
    led->init = gpio_led_init;
    led->set_on = gpio_led_set_on;
    led->set_off = gpio_led_set_off;
    led->toggle = gpio_led_toggle;
    led->is_on = gpio_led_is_on;
    led->set_brightness = NULL; // No PWM support
    led->set_rgb_color = NULL;  // Not RGB
    
    // Register LED
    g_leds[g_led_count++] = led;
    
    return led;
}

LedAbstraction* led_create_pwm(uint8_t led_id, uint8_t port_id, LedType type,
                              GPIO_TypeDef* gpio_port, uint16_t gpio_pin, bool active_high,
                              TIM_HandleTypeDef* htim, uint32_t channel) {
    LedAbstraction* led = led_create_simple(led_id, port_id, type, gpio_port, gpio_pin, active_high);
    if (!led) {
        return NULL;
    }
    
    // Add PWM configuration
    led->config.htim = htim;
    led->config.tim_channel = channel;
    led->config.pwm_enabled = true;
    led->set_brightness = pwm_led_set_brightness;
    
    return led;
}

LedAbstraction* led_create_rgb(uint8_t led_id, uint8_t port_id,
                              GPIO_TypeDef* r_port, uint16_t r_pin,
                              GPIO_TypeDef* g_port, uint16_t g_pin,
                              GPIO_TypeDef* b_port, uint16_t b_pin) {
    if (!g_abstraction_initialized || g_led_count >= MAX_LEDS) {
        return NULL;
    }
    
    LedAbstraction* led = (LedAbstraction*)calloc(1, sizeof(LedAbstraction));
    if (!led) {
        return NULL;
    }
    
    // Configure as RGB LED
    led->config.led_id = led_id;
    led->config.port_id = port_id;
    led->config.type = LED_TYPE_STATUS; // Default type
    led->config.color = LED_COLOR_RGB;
    led->config.pwm_enabled = false; // Assume simple on/off for now
    
    // RGB configuration
    led->config.rgb.red_port = r_port;
    led->config.rgb.red_pin = r_pin;
    led->config.rgb.green_port = g_port;
    led->config.rgb.green_pin = g_pin;
    led->config.rgb.blue_port = b_port;
    led->config.rgb.blue_pin = b_pin;
    
    // Initialize timing and state
    memcpy(&led->timing, &DEFAULT_TIMING, sizeof(LedTimingConfig));
    memset(&led->state, 0, sizeof(led->state));
    
    // Set function pointers
    led->init = gpio_led_init;
    led->set_on = gpio_led_set_on;
    led->set_off = gpio_led_set_off;
    led->toggle = gpio_led_toggle;
    led->is_on = gpio_led_is_on;
    led->set_rgb_color = rgb_led_set_color;
    
    // Register LED
    g_leds[g_led_count++] = led;
    
    return led;
}

bool led_destroy(LedAbstraction* led) {
    if (!led) {
        return false;
    }
    
    // Remove from registry
    for (uint8_t i = 0; i < g_led_count; i++) {
        if (g_leds[i] == led) {
            // Turn off LED before destroying
            if (led->set_off) {
                led->set_off(led);
            }
            
            // Shift remaining LEDs
            for (uint8_t j = i; j < g_led_count - 1; j++) {
                g_leds[j] = g_leds[j + 1];
            }
            g_leds[--g_led_count] = NULL;
            break;
        }
    }
    
    free(led);
    return true;
}

// LED control functions
bool led_initialize(LedAbstraction* led) {
    if (!led || !led->init) {
        return false;
    }
    
    return led->init(led);
}

bool led_set_state(LedAbstraction* led, LedState state) {
    if (!led || !led->config.initialized) {
        return false;
    }
    
    led->config.current_state = state;
    led->state.pattern_start_ms = HAL_GetTick();
    led->state.last_toggle_ms = led->state.pattern_start_ms;
    led->state.toggle_state = false;
    led->state.flash_count = 0;
    
    switch (state) {
        case LED_STATE_OFF:
            led->state.pattern_active = false;
            return led->set_off ? led->set_off(led) : false;
            
        case LED_STATE_ON:
            led->state.pattern_active = false;
            return led->set_on ? led->set_on(led) : false;
            
        case LED_STATE_BLINK_SLOW:
        case LED_STATE_BLINK_FAST:
        case LED_STATE_PULSE:
        case LED_STATE_FLASH:
            led->state.pattern_active = true;
            return true;
            
        default:
            return false;
    }
}

bool led_set_brightness(LedAbstraction* led, uint8_t brightness) {
    if (!led) {
        return false;
    }
    
    brightness = (brightness > 100) ? 100 : brightness; // Clamp to 0-100
    led->config.brightness = brightness;
    
    if (led->set_brightness) {
        return led->set_brightness(led, brightness);
    }
    
    return true; // Success for non-PWM LEDs (brightness stored but not applied)
}

bool led_set_color(LedAbstraction* led, uint8_t r, uint8_t g, uint8_t b) {
    if (!led || !led->set_rgb_color) {
        return false;
    }
    
    return led->set_rgb_color(led, r, g, b);
}

bool led_flash_once(LedAbstraction* led) {
    return led_flash_count(led, 1);
}

bool led_flash_count(LedAbstraction* led, uint32_t count) {
    if (!led) {
        return false;
    }
    
    led->state.flash_count = count;
    return led_set_state(led, LED_STATE_FLASH);
}

bool led_update(LedAbstraction* led) {
    if (!led || !led->config.initialized || !led->state.pattern_active) {
        return true; // Nothing to update
    }
    
    switch (led->config.current_state) {
        case LED_STATE_BLINK_SLOW:
        case LED_STATE_BLINK_FAST:
            led_update_blink_pattern(led);
            break;
            
        case LED_STATE_PULSE:
            led_update_pulse_pattern(led);
            break;
            
        case LED_STATE_FLASH:
            led_update_flash_pattern(led);
            break;
            
        default:
            break;
    }
    
    return true;
}

LedState led_get_state(LedAbstraction* led) {
    return led ? led->config.current_state : LED_STATE_OFF;
}

bool led_is_active(LedAbstraction* led) {
    if (!led) {
        return false;
    }
    
    return led->state.pattern_active || led->config.current_state == LED_STATE_ON;
}

// LED pattern control
bool led_start_pattern(LedAbstraction* led, LedState pattern) {
    return led_set_state(led, pattern);
}

bool led_stop_pattern(LedAbstraction* led) {
    return led_set_state(led, LED_STATE_OFF);
}

bool led_configure_timing(LedAbstraction* led, const LedTimingConfig* timing) {
    if (!led || !timing) {
        return false;
    }
    
    memcpy(&led->timing, timing, sizeof(LedTimingConfig));
    return true;
}

// Utility functions
const char* led_state_to_string(LedState state) {
    switch (state) {
        case LED_STATE_OFF: return "Off";
        case LED_STATE_ON: return "On";
        case LED_STATE_BLINK_SLOW: return "Blink Slow";
        case LED_STATE_BLINK_FAST: return "Blink Fast";
        case LED_STATE_PULSE: return "Pulse";
        case LED_STATE_FLASH: return "Flash";
        default: return "Unknown";
    }
}

const char* led_type_to_string(LedType type) {
    switch (type) {
        case LED_TYPE_STATUS: return "Status";
        case LED_TYPE_SIGNAL: return "Signal";
        case LED_TYPE_ERROR: return "Error";
        case LED_TYPE_ACTIVITY: return "Activity";
        default: return "Unknown";
    }
}

const char* led_color_to_string(LedColor color) {
    switch (color) {
        case LED_COLOR_RED: return "Red";
        case LED_COLOR_GREEN: return "Green";
        case LED_COLOR_BLUE: return "Blue";
        case LED_COLOR_YELLOW: return "Yellow";
        case LED_COLOR_WHITE: return "White";
        case LED_COLOR_RGB: return "RGB";
        default: return "Unknown";
    }
}

// Port-specific LED management
PortLedGroup* port_leds_create(uint8_t port_id) {
    if (!g_abstraction_initialized || port_id >= MAX_PORT_LED_GROUPS || g_port_groups[port_id]) {
        return NULL;
    }
    
    PortLedGroup* group = (PortLedGroup*)calloc(1, sizeof(PortLedGroup));
    if (!group) {
        return NULL;
    }
    
    group->port_id = port_id;
    g_port_groups[port_id] = group;
    
    return group;
}

bool port_leds_destroy(PortLedGroup* group) {
    if (!group) {
        return false;
    }
    
    // Clear from registry
    if (group->port_id < MAX_PORT_LED_GROUPS) {
        g_port_groups[group->port_id] = NULL;
    }
    
    // Don't destroy individual LEDs - they're managed separately
    free(group);
    return true;
}

bool port_leds_set_status(PortLedGroup* group, bool enabled) {
    if (!group || !group->status_led) {
        return false;
    }
    
    return led_set_state(group->status_led, enabled ? LED_STATE_ON : LED_STATE_OFF);
}

bool port_leds_set_signal_detected(PortLedGroup* group, bool detected) {
    if (!group || !group->signal_led) {
        return false;
    }
    
    return led_set_state(group->signal_led, detected ? LED_STATE_ON : LED_STATE_OFF);
}

bool port_leds_set_calculating(PortLedGroup* group, bool calculating) {
    if (!group || !group->activity_led) {
        return false;
    }
    
    return led_set_state(group->activity_led, calculating ? LED_STATE_BLINK_FAST : LED_STATE_OFF);
}

bool port_leds_set_error(PortLedGroup* group, bool error) {
    if (!group || !group->error_led) {
        return false;
    }
    
    return led_set_state(group->error_led, error ? LED_STATE_BLINK_SLOW : LED_STATE_OFF);
}

bool port_leds_update_all(PortLedGroup* group) {
    if (!group) {
        return false;
    }
    
    bool result = true;
    if (group->status_led) result &= led_update(group->status_led);
    if (group->signal_led) result &= led_update(group->signal_led);
    if (group->error_led) result &= led_update(group->error_led);
    if (group->activity_led) result &= led_update(group->activity_led);
    
    return result;
}

// Driver implementations
static bool gpio_led_init(LedAbstraction* led) {
    if (!led || !led->config.gpio_port) {
        return false;
    }
    
    // Configure GPIO pin as output
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = led->config.gpio_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(led->config.gpio_port, &GPIO_InitStruct);
    
    // Initialize to OFF state
    gpio_led_set_off(led);
    
    led->config.initialized = true;
    return true;
}

static bool gpio_led_set_on(LedAbstraction* led) {
    if (!led || !led->config.gpio_port || !led->config.initialized) {
        return false;
    }
    
    GPIO_PinState state = led->config.active_high ? GPIO_PIN_SET : GPIO_PIN_RESET;
    HAL_GPIO_WritePin(led->config.gpio_port, led->config.gpio_pin, state);
    
    return true;
}

static bool gpio_led_set_off(LedAbstraction* led) {
    if (!led || !led->config.gpio_port || !led->config.initialized) {
        return false;
    }
    
    GPIO_PinState state = led->config.active_high ? GPIO_PIN_RESET : GPIO_PIN_SET;
    HAL_GPIO_WritePin(led->config.gpio_port, led->config.gpio_pin, state);
    
    return true;
}

static bool gpio_led_toggle(LedAbstraction* led) {
    if (!led || !led->config.gpio_port || !led->config.initialized) {
        return false;
    }
    
    HAL_GPIO_TogglePin(led->config.gpio_port, led->config.gpio_pin);
    return true;
}

static bool gpio_led_is_on(LedAbstraction* led) {
    if (!led || !led->config.gpio_port || !led->config.initialized) {
        return false;
    }
    
    GPIO_PinState pin_state = HAL_GPIO_ReadPin(led->config.gpio_port, led->config.gpio_pin);
    return led->config.active_high ? (pin_state == GPIO_PIN_SET) : (pin_state == GPIO_PIN_RESET);
}

static bool pwm_led_set_brightness(LedAbstraction* led, uint8_t brightness) {
    if (!led || !led->config.htim || !led->config.pwm_enabled) {
        return false;
    }
    
    uint32_t pulse_value = (PWM_MAX_VALUE * brightness) / 100;
    
    switch (led->config.tim_channel) {
        case TIM_CHANNEL_1:
            __HAL_TIM_SET_COMPARE(led->config.htim, TIM_CHANNEL_1, pulse_value);
            break;
        case TIM_CHANNEL_2:
            __HAL_TIM_SET_COMPARE(led->config.htim, TIM_CHANNEL_2, pulse_value);
            break;
        case TIM_CHANNEL_3:
            __HAL_TIM_SET_COMPARE(led->config.htim, TIM_CHANNEL_3, pulse_value);
            break;
        case TIM_CHANNEL_4:
            __HAL_TIM_SET_COMPARE(led->config.htim, TIM_CHANNEL_4, pulse_value);
            break;
        default:
            return false;
    }
    
    led->state.current_brightness = brightness;
    return true;
}

static bool rgb_led_set_color(LedAbstraction* led, uint8_t r, uint8_t g, uint8_t b) {
    if (!led || led->config.color != LED_COLOR_RGB) {
        return false;
    }
    
    // Simple on/off control for RGB (no PWM assumed)
    GPIO_PinState red_state = r > 127 ? GPIO_PIN_SET : GPIO_PIN_RESET;
    GPIO_PinState green_state = g > 127 ? GPIO_PIN_SET : GPIO_PIN_RESET;
    GPIO_PinState blue_state = b > 127 ? GPIO_PIN_SET : GPIO_PIN_RESET;
    
    HAL_GPIO_WritePin(led->config.rgb.red_port, led->config.rgb.red_pin, red_state);
    HAL_GPIO_WritePin(led->config.rgb.green_port, led->config.rgb.green_pin, green_state);
    HAL_GPIO_WritePin(led->config.rgb.blue_port, led->config.rgb.blue_pin, blue_state);
    
    return true;
}

// Pattern generation functions
static void led_update_blink_pattern(LedAbstraction* led) {
    uint32_t current_ms = HAL_GetTick();
    uint16_t period = (led->config.current_state == LED_STATE_BLINK_SLOW) ? 
                      led->timing.slow_blink_period_ms : 
                      led->timing.fast_blink_period_ms;
    
    if (current_ms - led->state.last_toggle_ms >= (period / 2)) {
        if (led->toggle) {
            led->toggle(led);
        }
        led->state.last_toggle_ms = current_ms;
        led->state.toggle_state = !led->state.toggle_state;
    }
}

static void led_update_pulse_pattern(LedAbstraction* led) {
    uint32_t current_ms = HAL_GetTick();
    uint32_t elapsed = current_ms - led->state.pattern_start_ms;
    uint32_t period = led->timing.pulse_period_ms;
    
    // Calculate position in pulse cycle (0.0 to 1.0)
    float phase = (float)(elapsed % period) / (float)period;
    
    // Generate sine wave for breathing effect
    float brightness_factor = (sinf(2.0f * M_PI * phase) + 1.0f) / 2.0f;
    
    // Scale between min and max brightness
    uint8_t min_brightness = led->timing.pulse_min_brightness;
    uint8_t max_brightness = led->timing.pulse_max_brightness;
    uint8_t brightness = min_brightness + (uint8_t)((max_brightness - min_brightness) * brightness_factor);
    
    if (led->set_brightness) {
        led->set_brightness(led, brightness);
    } else {
        // For non-PWM LEDs, just turn on/off based on brightness threshold
        if (brightness > 50) {
            if (led->set_on) led->set_on(led);
        } else {
            if (led->set_off) led->set_off(led);
        }
    }
    
    led->state.current_brightness = brightness;
}

static void led_update_flash_pattern(LedAbstraction* led) {
    uint32_t current_ms = HAL_GetTick();
    uint32_t elapsed = current_ms - led->state.pattern_start_ms;
    
    if (led->state.flash_count > 0) {
        uint16_t flash_duration = led->timing.flash_duration_ms;
        uint32_t cycle_time = flash_duration * 2; // On time + off time
        
        if (elapsed < flash_duration) {
            // Flash on
            if (led->set_on) led->set_on(led);
        } else if (elapsed < cycle_time) {
            // Flash off
            if (led->set_off) led->set_off(led);
        } else {
            // Cycle complete, reduce count and restart
            led->state.flash_count--;
            led->state.pattern_start_ms = current_ms;
            
            if (led->state.flash_count == 0) {
                // All flashes complete, turn off pattern
                led->state.pattern_active = false;
                led->config.current_state = LED_STATE_OFF;
                if (led->set_off) led->set_off(led);
            }
        }
    }
}