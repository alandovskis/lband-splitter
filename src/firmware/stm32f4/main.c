#include "stm32f4xx_hal.h"
#include "frequency_detector.h"
#include "uart_protocol.h"
#include "port.h"
#include "display.h"
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

// Individual port instances
#define MAX_PORTS 32
static Port ports[MAX_PORTS];

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
  
  // Initialize all ports
  for (int i = 0; i < MAX_PORTS; i++) {
    port_init(&ports[i], i);
  }

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

    // Update all ports (LEDs and displays)
    for (int i = 0; i < MAX_PORTS; i++) {
      port_update(&ports[i]);
    }

    // Handle continuous measurement if enabled
    if (continuous_measurement && measurement_counter > 0) {
      // Note: port_id would need to be determined from command context
      uint8_t port_id = 0; // This would be extracted from command context
      port_set_calculation_active(&ports[port_id], true);
      FrequencyReading reading;
      if (frequency_detector_measure(&freq_detector_state, &reading)) {
        // Send reading via UART if requested
        uart_protocol_send_reading(&reading);
        
        // Store measurement data for autonomous display
        if (port_id < MAX_PORTS) {
          port_update_measurements(&ports[port_id], reading.frequency_mhz, reading.snr_db);
        }
        
        // Update signal detection based on reading quality
        port_set_signal_detection(&ports[port_id], reading.valid && reading.frequency_mhz > 950.0);
      }
      port_set_calculation_active(&ports[port_id], false);
      measurement_counter--;
    }

    // Power management - enter sleep mode if idle
    bool any_calculation_active = false;
    for (int i = 0; i < MAX_PORTS; i++) {
      if (ports[i].calculation_active) {
        any_calculation_active = true;
        break;
      }
    }
    if (!continuous_measurement && !any_calculation_active) {
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
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  // Initialize all GPIO pins for LEDs (all 32 ports)
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  
  // Status LEDs on GPIOA (pins 0-15) and GPIOC (pins 0-15)
  GPIO_InitStruct.Pin = GPIO_PIN_All;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  
  // Signal LEDs on GPIOB (pins 0-15) and GPIOD (pins 0-15)  
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
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
  port_set_calculation_active(&ports[port_id], true);
  FrequencyReading reading;
  if (frequency_detector_measure(&freq_detector_state, &reading)) {
    uart_protocol_send_reading(&reading);
    
    // Store measurement data for autonomous display
    if (port_id < MAX_PORTS) {
      port_update_measurements(&ports[port_id], reading.frequency_mhz, reading.snr_db);
    }
    
    // Update signal detection based on reading quality
    port_set_signal_detection(&ports[port_id], reading.valid && reading.frequency_mhz > 950.0);
  }
  port_set_calculation_active(&ports[port_id], false);
}

// Port control functions for daemon communication
void handle_enable_port(uint8_t port_id, bool enabled) {
  if (port_id >= MAX_PORTS) return;
  port_set_enabled(&ports[port_id], enabled);
}

void handle_signal_detection(uint8_t port_id, bool detected) {
  if (port_id >= MAX_PORTS) return;
  port_set_signal_detection(&ports[port_id], detected);
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

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {
  // User can add implementation here
}
#endif