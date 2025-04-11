#ifndef DWT_STM32_DELAY_H
#define DWT_STM32_DELAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f10x.h"    // SPL library for STM32F1 series
#include "stm32f10x_rcc.h" // SPL library for RCC
/**
 * @brief Function to get the SystemCoreClock in SPL
 * This function uses RCC_GetClocksFreq from SPL to retrieve HCLK frequency.
 * @return The HCLK frequency in Hz.
 */
static inline uint32_t Get_SystemCoreClock(void) {
    RCC_ClocksTypeDef clocks;
    RCC_GetClocksFreq(&clocks);
    return clocks.HCLK_Frequency;  // Return the HCLK frequency
}

/**
 * @brief Initializes DWT_Cycle_Count for DWT_Delay_us function
 * @return Error DWT counter
 * 1: DWT counter Error
 * 0: DWT counter works
 */
uint32_t DWT_Delay_Init(void);

/**
 * @brief Provides a delay (in microseconds)
 * @param microseconds: delay in microseconds
 */
__STATIC_INLINE void DWT_Delay_us(volatile uint32_t microseconds) {
    DWT->CYCCNT = 0;
    microseconds *= (SystemCoreClock / 1000000);
    while (DWT->CYCCNT < microseconds);
}

/**
 * @brief Provides a delay (in milliseconds)
 * @param milliseconds: delay in milliseconds
 */
void DWT_Delay_ms(volatile uint32_t milliseconds);

/**
 * @brief Initializes the system delay functionality using SysTick.
 * This function configures the SysTick timer to generate interrupts every 1 millisecond.
 */
void Sys_DelayInit(void);

/**
 * @brief Provides a delay for the specified time in milliseconds.
 * The function will block for the given amount of time.
 *
 * @param milliseconds: Number of milliseconds to delay the execution
 */
void Sys_DelayMs(uint32_t milliseconds);


extern volatile uint8_t Timer1, Timer2;   

void SDTimer_Handler(void);

void Delay_Us(uint32_t Time);
void Delay_Ms(uint32_t Time);
void TIM2_Config(void);

#ifdef __cplusplus
}
#endif

#endif // DWT_STM32_DELAY_H
