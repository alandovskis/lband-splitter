#include "stm32f4xx_hal.h"
#include "frequency_detector.h"
#include "uart_protocol.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// System Clock Configuration
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
static void MX_I2C1_Init(void);

// Global handles
UART_HandleTypeDef huart2;
ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim2;
I2C_HandleTypeDef hi2c1;

// Frequency detection state
static FrequencyDetectorState freq_detector_state;
static volatile bool continuous_measurement = false;
static volatile uint32_t measurement_counter = 0;

int main(void) {
  // Initialize HAL
  HAL_Init();

  // Configure the system clock
  SystemClock_Config();

  // Initialize peripherals
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_I2C1_Init();
  
  // Initialize global display system
  display_init();
  
  // Initialize individual port hardware (LEDs and displays)
  init_port_hardware();

  // Initialize frequency detector
  frequency_detector_init(&freq_detector_state);

  // Start ADC calibration
  HAL_ADCEx_Calibration_Start(&hadc1);

  // Start timer for periodic measurements
  HAL_TIM_Base_Start_IT(&htim2);

  // Main communication loop
  while (1) {
    // Process UART commands
    uart_protocol_process();

    // Handle autonomous LED control
    handle_autonomous_leds();
    
    // Handle autonomous display control for all individual ports
    handle_autonomous_displays();

    // Handle continuous measurement if enabled
    if (continuous_measurement && measurement_counter > 0) {
      // Note: port_id would need to be determined from context
      uint8_t port_id = 0; // This would be extracted from command context
      handle_calculation_start(port_id);
      FrequencyReading reading;
      if (frequency_detector_measure(&freq_detector_state, &reading)) {
        // Send reading via UART if requested
        uart_protocol_send_reading(&reading);
        
        // Store measurement data for autonomous display
        if (port_id < MAX_PORTS) {
          ports[port_id].last_frequency_mhz = reading.frequency_mhz;
          ports[port_id].last_snr_db = reading.snr_db;
        }
        
        // Update signal detection based on reading quality
        handle_signal_detection(port_id, reading.valid && reading.frequency_mhz > 950.0);
      }
      handle_calculation_complete(port_id);
      measurement_counter--;
    }

    // Power management - enter sleep mode if idle
    if (!continuous_measurement && !calculation_active) {
      HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    }
  }
}

// Timer interrupt handler for periodic measurements
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM2) {
    if (continuous_measurement) {
      measurement_counter++;
    }
  }
}

// ADC conversion complete callback
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
  if (hadc->Instance == ADC1) {
    frequency_detector_adc_callback(&freq_detector_state,
                                    HAL_ADC_GetValue(hadc));
  }
}

// System Clock Configuration
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;   // 8 MHz HSE / 8 = 1 MHz
  RCC_OscInitStruct.PLL.PLLN = 336; // 1 MHz * 336 = 336 MHz
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2; // 336 MHz / 2 = 168 MHz
  RCC_OscInitStruct.PLL.PLLQ = 7;              // 336 MHz / 7 = 48 MHz
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;   // 168 MHz
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;    // 42 MHz
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;    // 84 MHz

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
    Error_Handler();
  }
}

// USART2 Initialization
static void MX_USART2_UART_Init(void) {
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK) {
    Error_Handler();
  }
}

// ADC1 Initialization
static void MX_ADC1_Init(void) {
  ADC_ChannelConfTypeDef sConfig = {0};

  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK) {
    Error_Handler();
  }

  // Configure ADC channel for L-band signal detection
  sConfig.Channel = ADC_CHANNEL_0; // PA0
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
    Error_Handler();
  }
}

// TIM2 Initialization for periodic measurements
static void MX_TIM2_Init(void) {
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 42000 - 1;   // 42 MHz / 42000 = 1 kHz
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 100 - 1;       // 1 kHz / 100 = 10 Hz (100ms period)
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
    Error_Handler();
  }

  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) {
    Error_Handler();
  }
}

// GPIO Initialization
static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // Enable GPIO Clocks
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  // Configure GPIO pin : PC13 (User LED)
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  // Configure GPIO pins for LED control (Status and Signal LEDs)
  GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6;  // Status LED (PA5), Signal LED (PA6)
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // Configure GPIO pins for RF switching control
  GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_8;  // RF control pins
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

// Error Handler
void Error_Handler(void) {
  __disable_irq();
  while (1) {
    // Toggle LED to indicate error
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    HAL_Delay(100);
  }
}

// I2C1 Initialization for display communication
static void MX_I2C1_Init(void) {
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
    Error_Handler();
  }
}

// Display control state and functions
#define DISPLAY_I2C_ADDRESS 0x3C  // Standard SSD1306 OLED address
static volatile bool display_initialized = false;
static volatile uint8_t display_brightness = 100;

typedef struct {
  double frequency_mhz;
  double snr_db;
  bool signal_present;
  char custom_text[32];
  uint8_t brightness;
} DisplayData;

static DisplayData current_display_data;

void display_init(void) {
  // Initialize SSD1306 OLED display
  // This is a simplified initialization - real implementation would include
  // full SSD1306 command sequence
  uint8_t init_commands[] = {
    0x00, 0xAE,  // Display OFF
    0x00, 0x20, 0x00,  // Set Memory Addressing Mode
    0x00, 0xB0,  // Set Page Start Address
    0x00, 0xC8,  // Set COM Output Scan Direction
    0x00, 0x00,  // Set low column address
    0x00, 0x10,  // Set high column address
    0x00, 0x40,  // Set start line address
    0x00, 0x81, 0x7F,  // Set contrast control register
    0x00, 0xA1,  // Set segment re-map
    0x00, 0xA6,  // Set normal display
    0x00, 0xA8, 0x3F,  // Set multiplex ratio
    0x00, 0xA4,  // Output RAM to Display
    0x00, 0xD3, 0x00,  // Set display offset
    0x00, 0xD5, 0xF0,  // Set display clock divide ratio
    0x00, 0xD9, 0x22,  // Set pre-charge period
    0x00, 0xDA, 0x12,  // Set com pins hardware configuration
    0x00, 0xDB, 0x20,  // Set vcomh
    0x00, 0x8D, 0x14,  // Set DC-DC enable
    0x00, 0xAF   // Display ON
  };

  if (HAL_I2C_Transmit(&hi2c1, DISPLAY_I2C_ADDRESS << 1, init_commands, 
                       sizeof(init_commands), HAL_MAX_DELAY) == HAL_OK) {
    display_initialized = true;
    display_clear();
    display_show_startup_message();
  }
}

void display_clear(void) {
  if (!display_initialized) return;
  
  // Clear display buffer command
  uint8_t clear_cmd[] = {0x00, 0x21, 0x00, 0x7F, 0x22, 0x00, 0x07};
  HAL_I2C_Transmit(&hi2c1, DISPLAY_I2C_ADDRESS << 1, clear_cmd, 
                   sizeof(clear_cmd), HAL_MAX_DELAY);
  
  // Send zeros to clear all pages
  for (int page = 0; page < 8; page++) {
    uint8_t data[129];
    data[0] = 0x40;  // Data mode
    memset(&data[1], 0, 128);  // Clear 128 columns
    HAL_I2C_Transmit(&hi2c1, DISPLAY_I2C_ADDRESS << 1, data, 
                     sizeof(data), HAL_MAX_DELAY);
  }
}

void display_show_startup_message(void) {
  if (!display_initialized) return;
  
  // Simple text display - real implementation would use font rendering
  display_print_text(0, 0, "L-Band Splitter");
  display_print_text(0, 2, "STM32F4 Ready");
  display_print_text(0, 4, "Freq: ----.-- MHz");
  display_print_text(0, 6, "SNR:  --.- dB");
}

void display_print_text(uint8_t x, uint8_t y, const char* text) {
  if (!display_initialized) return;
  
  // Simplified text rendering - real implementation would use character bitmaps
  uint8_t cmd[] = {0x00, 0x21, x, x + strlen(text) * 6, 0x22, y, y};
  HAL_I2C_Transmit(&hi2c1, DISPLAY_I2C_ADDRESS << 1, cmd, sizeof(cmd), HAL_MAX_DELAY);
  
  // Send character data (simplified - would normally render from font)
  uint8_t data[2];
  data[0] = 0x40;  // Data mode
  for (const char* c = text; *c; c++) {
    // Simple character pattern (would use proper font in real implementation)
    data[1] = 0xFF;  // Full column for visibility
    HAL_I2C_Transmit(&hi2c1, DISPLAY_I2C_ADDRESS << 1, data, 2, HAL_MAX_DELAY);
  }
}

void display_update_frequency_data(double frequency_mhz, double snr_db, bool signal_present) {
  current_display_data.frequency_mhz = frequency_mhz;
  current_display_data.snr_db = snr_db;
  current_display_data.signal_present = signal_present;
  
  if (!display_initialized) return;
  
  char freq_str[32];
  char snr_str[32];
  
  if (signal_present && frequency_mhz > 0) {
    snprintf(freq_str, sizeof(freq_str), "Freq: %7.2f MHz", frequency_mhz);
    snprintf(snr_str, sizeof(snr_str), "SNR:  %5.1f dB", snr_db);
  } else {
    snprintf(freq_str, sizeof(freq_str), "Freq: ----.-- MHz");
    snprintf(snr_str, sizeof(snr_str), "SNR:  --.- dB");
  }
  
  display_print_text(0, 4, freq_str);
  display_print_text(0, 6, snr_str);
}

void display_set_brightness(uint8_t brightness) {
  display_brightness = brightness;
  if (!display_initialized) return;
  
  // Set contrast (brightness) command
  uint8_t contrast_cmd[] = {0x00, 0x81, (brightness * 255) / 100};
  HAL_I2C_Transmit(&hi2c1, DISPLAY_I2C_ADDRESS << 1, contrast_cmd, 
                   sizeof(contrast_cmd), HAL_MAX_DELAY);
}

void display_show_custom_text(const char* text) {
  if (!display_initialized) return;
  
  strncpy(current_display_data.custom_text, text, sizeof(current_display_data.custom_text) - 1);
  current_display_data.custom_text[sizeof(current_display_data.custom_text) - 1] = '\0';
  
  display_clear();
  display_print_text(0, 0, "L-Band Splitter");
  display_print_text(0, 2, text);
}

// Individual port state for autonomous LED control
#define MAX_PORTS 32

typedef enum {
  PORT_DISABLED = 0,
  PORT_ENABLED_NO_SIGNAL = 1,
  PORT_ENABLED_CALCULATING = 2,
  PORT_ENABLED_LOCKED = 3
} PortState;

typedef struct {
  PortState state;
  bool enabled;
  bool signal_detected;
  bool calculation_active;
  uint32_t last_blink_time;
  uint32_t blink_counter;
  GPIO_TypeDef* status_led_port;
  uint16_t status_led_pin;
  GPIO_TypeDef* signal_led_port;
  uint16_t signal_led_pin;
  // Individual display state for autonomous control
  uint8_t display_i2c_address;    // I2C address for this port's display
  uint8_t display_mux_channel;    // I2C multiplexer channel
  double last_frequency_mhz;
  double last_snr_db;
  uint32_t last_display_update;
  bool display_showing_frequency;
  bool display_initialized;
} PortControlState;

// Display configuration
#define DISPLAY_ALTERNATION_PERIOD_MS 2000
#define I2C_MUX_ADDRESS 0x70  // TCA9548A I2C multiplexer base address
#define DISPLAY_BASE_ADDRESS 0x3C  // SSD1306 OLED base address

static volatile PortControlState ports[MAX_PORTS];

// LED control functions
// I2C multiplexer control functions for individual displays
bool select_display_mux_channel(uint8_t mux_address, uint8_t channel) {
  if (channel > 7) return false;  // TCA9548A has 8 channels per IC
  
  uint8_t mux_data = (1 << channel);  // Enable specific channel
  return HAL_I2C_Transmit(&hi2c1, mux_address << 1, &mux_data, 1, HAL_MAX_DELAY) == HAL_OK;
}

void disable_display_mux(uint8_t mux_address) {
  uint8_t mux_data = 0x00;  // Disable all channels
  HAL_I2C_Transmit(&hi2c1, mux_address << 1, &mux_data, 1, HAL_MAX_DELAY);
}

// Initialize individual display for a specific port
bool init_port_display(uint8_t port_id) {
  if (port_id >= MAX_PORTS) return false;
  
  // Select the correct I2C multiplexer channel for this port's display
  if (!select_display_mux_channel(ports[port_id].display_mux_channel / 8 + I2C_MUX_ADDRESS, 
                                   ports[port_id].display_mux_channel % 8)) {
    return false;
  }
  
  // Initialize SSD1306 display on this channel
  uint8_t init_commands[] = {
    0x00, 0xAE,  // Display OFF
    0x00, 0x20, 0x00,  // Set Memory Addressing Mode
    0x00, 0xB0,  // Set Page Start Address
    0x00, 0xC8,  // Set COM Output Scan Direction
    0x00, 0x00,  // Set Low Column Start Address
    0x00, 0x10,  // Set High Column Start Address
    0x00, 0x40,  // Set Display Start Line
    0x00, 0x81, 0x7F,  // Set Contrast Control
    0x00, 0xA1,  // Set Segment Re-map
    0x00, 0xA6,  // Set Normal Display
    0x00, 0xA8, 0x3F,  // Set Multiplex Ratio
    0x00, 0xA4,  // Output follows RAM content
    0x00, 0xD3, 0x00,  // Set Display Offset
    0x00, 0xD5, 0xF0,  // Set Display Clock Divide Ratio
    0x00, 0xD9, 0x22,  // Set Precharge Period
    0x00, 0xDA, 0x12,  // Set COM Pins Configuration
    0x00, 0xDB, 0x20,  // Set VCOMH Deselect Level
    0x00, 0x8D, 0x14,  // Enable charge pump
    0x00, 0xAF   // Display ON
  };
  
  bool success = HAL_I2C_Transmit(&hi2c1, ports[port_id].display_i2c_address << 1, 
                                  init_commands, sizeof(init_commands), HAL_MAX_DELAY) == HAL_OK;
  
  if (success) {
    ports[port_id].display_initialized = true;
    // Clear the display initially
    clear_port_display(port_id);
  }
  
  // Disable multiplexer channel when done
  disable_display_mux(ports[port_id].display_mux_channel / 8 + I2C_MUX_ADDRESS);
  
  return success;
}

// Initialize port hardware (LEDs and displays)
void init_port_hardware(void) {
  // Initialize GPIO pin mappings and display addressing for all 32 ports
  for (int i = 0; i < MAX_PORTS; i++) {
    ports[i].state = PORT_DISABLED;
    ports[i].enabled = false;
    ports[i].signal_detected = false;
    ports[i].calculation_active = false;
    ports[i].last_blink_time = 0;
    ports[i].blink_counter = 0;
    ports[i].last_frequency_mhz = 0.0;
    ports[i].last_snr_db = 0.0;
    ports[i].last_display_update = 0;
    ports[i].display_showing_frequency = true;
    ports[i].display_initialized = false;
    
    // Configure I2C multiplexer and display addressing
    ports[i].display_mux_channel = i;  // Each port has its own mux channel
    ports[i].display_i2c_address = DISPLAY_BASE_ADDRESS;  // All displays use same address on different mux channels
    
    // Configure LED GPIO mappings - would need to match actual hardware layout
    if (i < 16) {
      ports[i].status_led_port = GPIOA;
      ports[i].status_led_pin = GPIO_PIN_0 << i;
      ports[i].signal_led_port = GPIOB;
      ports[i].signal_led_pin = GPIO_PIN_0 << i;
    } else {
      ports[i].status_led_port = GPIOC;
      ports[i].status_led_pin = GPIO_PIN_0 << (i - 16);
      ports[i].signal_led_port = GPIOD;
      ports[i].signal_led_pin = GPIO_PIN_0 << (i - 16);
    }
    
    // Initialize each port's display
    init_port_display(i);
  }
}

void set_port_status_led(uint8_t port_id, bool on) {
  if (port_id >= MAX_PORTS) return;
  
  if (on) {
    HAL_GPIO_WritePin(ports[port_id].status_led_port, ports[port_id].status_led_pin, GPIO_PIN_SET);
  } else {
    HAL_GPIO_WritePin(ports[port_id].status_led_port, ports[port_id].status_led_pin, GPIO_PIN_RESET);
  }
}

void set_port_signal_led(uint8_t port_id, bool on) {
  if (port_id >= MAX_PORTS) return;
  
  if (on) {
    HAL_GPIO_WritePin(ports[port_id].signal_led_port, ports[port_id].signal_led_pin, GPIO_PIN_SET);
  } else {
    HAL_GPIO_WritePin(ports[port_id].signal_led_port, ports[port_id].signal_led_pin, GPIO_PIN_RESET);
  }
}

// Update individual port state based on current conditions
void update_port_state(uint8_t port_id) {
  if (port_id >= MAX_PORTS) return;
  
  if (!ports[port_id].enabled) {
    ports[port_id].state = PORT_DISABLED;
  } else if (ports[port_id].calculation_active) {
    ports[port_id].state = PORT_ENABLED_CALCULATING;
  } else if (ports[port_id].signal_detected) {
    ports[port_id].state = PORT_ENABLED_LOCKED;
  } else {
    ports[port_id].state = PORT_ENABLED_NO_SIGNAL;
  }
}

// Autonomous LED control for individual port
void handle_port_autonomous_leds(uint8_t port_id) {
  if (port_id >= MAX_PORTS) return;
  
  update_port_state(port_id);
  
  uint32_t current_time = HAL_GetTick();
  
  switch (ports[port_id].state) {
    case PORT_DISABLED:
      // Both LEDs off when port is disabled
      set_port_status_led(port_id, false);
      set_port_signal_led(port_id, false);
      break;
      
    case PORT_ENABLED_NO_SIGNAL:
      // Status LED on, signal LED off when enabled but no signal
      set_port_status_led(port_id, true);
      set_port_signal_led(port_id, false);
      break;
      
    case PORT_ENABLED_CALCULATING:
      // Status LED on, signal LED blinking when calculating
      set_port_status_led(port_id, true);
      if ((current_time - ports[port_id].last_blink_time) >= 500) { // 500ms blink period
        ports[port_id].blink_counter++;
        set_port_signal_led(port_id, ports[port_id].blink_counter % 2 == 0);
        ports[port_id].last_blink_time = current_time;
      }
      break;
      
    case PORT_ENABLED_LOCKED:
      // Both LEDs on when locked
      set_port_status_led(port_id, true);
      set_port_signal_led(port_id, true);
      break;
  }
}

// Clear individual port display
void clear_port_display(uint8_t port_id) {
  if (port_id >= MAX_PORTS || !ports[port_id].display_initialized) return;
  
  // Select the correct I2C multiplexer channel
  if (!select_display_mux_channel(ports[port_id].display_mux_channel / 8 + I2C_MUX_ADDRESS,
                                   ports[port_id].display_mux_channel % 8)) {
    return;
  }
  
  // Clear display command
  uint8_t clear_cmd[] = {0x00, 0x01};  // Clear display command
  HAL_I2C_Transmit(&hi2c1, ports[port_id].display_i2c_address << 1, clear_cmd, sizeof(clear_cmd), HAL_MAX_DELAY);
  
  // Disable multiplexer channel
  disable_display_mux(ports[port_id].display_mux_channel / 8 + I2C_MUX_ADDRESS);
}

// Update individual port display with text
void update_port_display(uint8_t port_id, const char* line1, const char* line2) {
  if (port_id >= MAX_PORTS || !ports[port_id].display_initialized) return;
  
  // Select the correct I2C multiplexer channel
  if (!select_display_mux_channel(ports[port_id].display_mux_channel / 8 + I2C_MUX_ADDRESS,
                                   ports[port_id].display_mux_channel % 8)) {
    return;
  }
  
  // This is a simplified display update - actual implementation would need
  // proper SSD1306 text positioning and character rendering
  char display_buffer[64];
  snprintf(display_buffer, sizeof(display_buffer), "Port %d\n%s\n%s", port_id + 1,
           line1 ? line1 : "", line2 ? line2 : "");
  
  // Send display data (simplified - actual implementation needs proper SSD1306 protocol)
  uint8_t display_data[2] = {0x40, 0x00};  // Data mode
  HAL_I2C_Transmit(&hi2c1, ports[port_id].display_i2c_address << 1, display_data, 2, HAL_MAX_DELAY);
  
  // Disable multiplexer channel
  disable_display_mux(ports[port_id].display_mux_channel / 8 + I2C_MUX_ADDRESS);
}

// Autonomous display control for individual ports - each shows its own measurements
void handle_autonomous_displays(void) {
  uint32_t current_time = HAL_GetTick();
  
  for (int i = 0; i < MAX_PORTS; i++) {
    if (!ports[i].display_initialized) continue;
    
    switch (ports[i].state) {
      case PORT_DISABLED:
      case PORT_ENABLED_NO_SIGNAL:
        // Clear display when port is disabled or has no signal
        clear_port_display(i);
        break;
        
      case PORT_ENABLED_CALCULATING:
        // Show "Calculating..." or similar when measuring
        update_port_display(i, "Measuring", "Please wait...");
        break;
        
      case PORT_ENABLED_LOCKED:
        if (ports[i].last_frequency_mhz > 0.0) {
          // Alternate between frequency and SNR every 2 seconds
          if ((current_time - ports[i].last_display_update) >= DISPLAY_ALTERNATION_PERIOD_MS) {
            ports[i].display_showing_frequency = !ports[i].display_showing_frequency;
            ports[i].last_display_update = current_time;
          }
          
          char line1[32], line2[32];
          if (ports[i].display_showing_frequency) {
            snprintf(line1, sizeof(line1), "%.1f MHz", ports[i].last_frequency_mhz);
            snprintf(line2, sizeof(line2), "Frequency");
          } else {
            snprintf(line1, sizeof(line1), "%.1f dB", ports[i].last_snr_db);
            snprintf(line2, sizeof(line2), "SNR");
          }
          
          update_port_display(i, line1, line2);
        }
        break;
    }
  }
}

// Handle autonomous LED control for all ports
void handle_autonomous_leds(void) {
  for (int i = 0; i < MAX_PORTS; i++) {
    handle_port_autonomous_leds(i);
  }
}

// Command handlers called by UART protocol
void handle_start_continuous_measurement(void) {
  continuous_measurement = true;
  measurement_counter = 0;
}

void handle_stop_continuous_measurement(void) {
  continuous_measurement = false;
  measurement_counter = 0;
}

void handle_single_measurement(uint8_t port_id) {
  handle_calculation_start(port_id);
  FrequencyReading reading;
  if (frequency_detector_measure(&freq_detector_state, &reading)) {
    uart_protocol_send_reading(&reading);
    
    // Store measurement data for autonomous display
    if (port_id < MAX_PORTS) {
      ports[port_id].last_frequency_mhz = reading.frequency_mhz;
      ports[port_id].last_snr_db = reading.snr_db;
    }
    
    // Update signal detection based on reading quality
    handle_signal_detection(port_id, reading.valid && reading.frequency_mhz > 950.0);
  }
  handle_calculation_complete(port_id);
}

// Port control functions for daemon communication
void handle_enable_port(uint8_t port_id, bool enabled) {
  if (port_id >= MAX_PORTS) return;
  ports[port_id].enabled = enabled;
}

void handle_signal_detection(uint8_t port_id, bool detected) {
  if (port_id >= MAX_PORTS) return;
  ports[port_id].signal_detected = detected;
}

void handle_calculation_start(uint8_t port_id) {
  if (port_id >= MAX_PORTS) return;
  ports[port_id].calculation_active = true;
}

void handle_calculation_complete(uint8_t port_id) {
  if (port_id >= MAX_PORTS) return;
  ports[port_id].calculation_active = false;
}


#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {
  // User can add implementation here
}
#endif