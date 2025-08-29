/**
 * @file bootloader_main.c
 * @brief Secure bootloader main entry point for STM32F4
 * 
 * This file contains the main secure bootloader implementation that:
 * - Verifies firmware authenticity and integrity
 * - Implements rollback protection
 * - Provides emergency recovery mode
 * - Enforces hardware security features
 */

#include "stm32f4xx_hal.h"
#include "secure_boot.h"
#include <stdbool.h>

// System Clock Configuration for bootloader
void Bootloader_SystemClock_Config(void);
static void Bootloader_GPIO_Init(void);
static void Bootloader_UART_Init(void);

// Global handles for bootloader
UART_HandleTypeDef bootloader_uart;

// Bootloader status LED functions
static void bootloader_led_init(void);
static void bootloader_led_set(bool on);
static void bootloader_led_blink(uint32_t count, uint32_t delay_ms);

/**
 * @brief Bootloader main entry point
 * @return Should never return
 */
int main(void) {
    SecureBootResult result;
    bool recovery_mode = false;
    
    // Initialize HAL
    HAL_Init();
    
    // Configure system clock for bootloader
    Bootloader_SystemClock_Config();
    
    // Initialize peripherals needed for bootloader
    Bootloader_GPIO_Init();
    Bootloader_UART_Init();
    bootloader_led_init();
    
    // Initialize secure boot system
    result = secure_boot_init();
    if (result != SECURE_BOOT_OK) {
        bootloader_led_blink(3, 200); // 3 fast blinks = init error
        secure_boot_emergency_recovery();
    }
    
    // Check if we need to enter recovery mode
    if (secure_boot_is_recovery_needed()) {
        recovery_mode = true;
        bootloader_led_blink(5, 100); // 5 very fast blinks = recovery mode
    }
    
    // Check for recovery mode trigger (e.g., button press during boot)
    // GPIO check for recovery button (implementation specific)
    if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET) {
        recovery_mode = true;
    }
    
    if (recovery_mode) {
        secure_boot_emergency_recovery();
        // Never returns
    }
    
    // Select firmware bank using A/B system
    FirmwareBankId selected_bank;
    bootloader_led_set(true); // LED on during bank selection
    
    result = secure_boot_select_boot_bank(&selected_bank);
    if (result != SECURE_BOOT_OK) {
        bootloader_led_blink(8, 300); // 8 medium blinks = bank selection failed
        secure_boot_emergency_recovery();
    }
    
    uint32_t selected_bank_addr = secure_boot_get_bank_address(selected_bank);
    if (selected_bank_addr == 0) {
        bootloader_led_blink(9, 250); // 9 blinks = invalid bank address
        secure_boot_emergency_recovery();
    }
    
    // Verify selected firmware bank
    result = secure_boot_verify_firmware(selected_bank_addr);
    
    if (result != SECURE_BOOT_OK) {
        // Verification failed - mark bank as failed and try fallback
        bootloader_led_blink(10, 500); // 10 slow blinks = verification failed
        
        secure_boot_mark_bank_failed(selected_bank);
        
        // Try to select another bank
        FirmwareBankId primary_bank, fallback_bank;
        if (secure_boot_verify_both_banks(&primary_bank, &fallback_bank) == SECURE_BOOT_OK) {
            if (fallback_bank != FIRMWARE_BANK_INVALID && fallback_bank != selected_bank) {
                selected_bank = fallback_bank;
                selected_bank_addr = secure_boot_get_bank_address(selected_bank);
                result = secure_boot_verify_firmware(selected_bank_addr);
            }
        }
        
        if (result != SECURE_BOOT_OK) {
            // Check if we should enter recovery mode
            if (secure_boot_is_recovery_needed()) {
                secure_boot_emergency_recovery();
            }
            
            // Both banks failed, enter recovery
            secure_boot_emergency_recovery();
        }
    }
    
    // Mark successful bank selection and verification
    secure_boot_mark_bank_successful(selected_bank);
    
    // Firmware verified successfully
    bootloader_led_set(false); // LED off before jumping
    
    // Enable hardware security features before jumping to application
    secure_boot_enable_flash_protection();
    
    #ifndef SECURE_BOOT_ENABLE_DEBUG
    secure_boot_disable_jtag_debug();
    #endif
    
    // Add a small delay to ensure LED state is visible
    HAL_Delay(100);
    
    // Jump to verified application
    result = secure_boot_jump_to_application(selected_bank_addr);
    
    // Should never reach here if jump was successful
    bootloader_led_blink(20, 100); // Rapid blinking = jump failed
    secure_boot_emergency_recovery();
    
    // Never returns
    while (1) {
        HAL_Delay(1000);
    }
}

/**
 * @brief System Clock Configuration for bootloader
 */
void Bootloader_SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    
    // Configure the main internal regulator output voltage
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    
    // Initialize the RCC Oscillators according to the specified parameters
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;  // 8 MHz HSE / 8 = 1 MHz
    RCC_OscInitStruct.PLL.PLLN = 168; // 1 MHz * 168 = 168 MHz
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2; // 168 MHz / 2 = 84 MHz (conservative for bootloader)
    RCC_OscInitStruct.PLL.PLLQ = 7;  // 168 MHz / 7 = 24 MHz
    
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }
    
    // Initialize the CPU, AHB and APB buses clocks
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                 |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief GPIO Initialization for bootloader
 */
static void Bootloader_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // GPIO Ports Clock Enable
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    // Configure status LED (PA5 - onboard LED on many STM32F4 boards)
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // Configure recovery button input (PC13 - user button on many boards)
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    // Initial LED state (off)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
}

/**
 * @brief UART Initialization for bootloader debug/recovery
 */
static void Bootloader_UART_Init(void) {
    // Enable UART clock
    __HAL_RCC_USART2_CLK_ENABLE();
    
    bootloader_uart.Instance = USART2;
    bootloader_uart.Init.BaudRate = 115200;
    bootloader_uart.Init.WordLength = UART_WORDLENGTH_8B;
    bootloader_uart.Init.StopBits = UART_STOPBITS_1;
    bootloader_uart.Init.Parity = UART_PARITY_NONE;
    bootloader_uart.Init.Mode = UART_MODE_TX_RX;
    bootloader_uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    bootloader_uart.Init.OverSampling = UART_OVERSAMPLING_16;
    
    if (HAL_UART_Init(&bootloader_uart) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief Initialize status LED
 */
static void bootloader_led_init(void) {
    // LED initialization already done in Bootloader_GPIO_Init()
    bootloader_led_set(false);
}

/**
 * @brief Set status LED state
 * @param on: true to turn LED on, false to turn off
 */
static void bootloader_led_set(bool on) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
 * @brief Blink status LED
 * @param count: number of blinks
 * @param delay_ms: delay between on/off states in milliseconds
 */
static void bootloader_led_blink(uint32_t count, uint32_t delay_ms) {
    for (uint32_t i = 0; i < count; i++) {
        bootloader_led_set(true);
        HAL_Delay(delay_ms);
        bootloader_led_set(false);
        HAL_Delay(delay_ms);
    }
}

/**
 * @brief Error Handler
 */
void Error_Handler(void) {
    // Disable interrupts
    __disable_irq();
    
    // Flash LED rapidly to indicate error
    while (1) {
        bootloader_led_blink(1, 50);
    }
}

/**
 * @brief System tick handler (required by HAL)
 */
void SysTick_Handler(void) {
    HAL_IncTick();
}

/**
 * @brief NMI Handler
 */
void NMI_Handler(void) {
    // Secure boot should handle NMI appropriately
    Error_Handler();
}

/**
 * @brief Hard Fault Handler
 */
void HardFault_Handler(void) {
    // In secure boot context, hard faults are critical
    Error_Handler();
}

/**
 * @brief Memory Management Fault Handler
 */
void MemManage_Handler(void) {
    Error_Handler();
}

/**
 * @brief Bus Fault Handler
 */
void BusFault_Handler(void) {
    Error_Handler();
}

/**
 * @brief Usage Fault Handler
 */
void UsageFault_Handler(void) {
    Error_Handler();
}