#include "stm32f4xx_it.h"

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim2;
extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef huart2;

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/

void NMI_Handler(void) {
  while (1) {
  }
}

void HardFault_Handler(void) {
  while (1) {
  }
}

void MemManage_Handler(void) {
  while (1) {
  }
}

void BusFault_Handler(void) {
  while (1) {
  }
}

void UsageFault_Handler(void) {
  while (1) {
  }
}

void SVC_Handler(void) {
}

void DebugMon_Handler(void) {
}

void PendSV_Handler(void) {
}

void SysTick_Handler(void) {
  HAL_IncTick();
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/******************************************************************************/

void TIM2_IRQHandler(void) {
  HAL_TIM_IRQHandler(&htim2);
}

void ADC_IRQHandler(void) {
  HAL_ADC_IRQHandler(&hadc1);
}

void USART2_IRQHandler(void) {
  HAL_UART_IRQHandler(&huart2);
}