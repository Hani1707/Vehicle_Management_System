#include "delay.h"

/**
 * @brief Initializes DWT_Clock_Cycle_Count for DWT_Delay_us function
 * @return Error DWT counter
 * 1: clock cycle counter not started
 * 0: clock cycle counter works
 */
uint32_t DWT_Delay_Init(void) {
 /* Disable TRC */
 CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk; // ~0x01000000;
 /* Enable TRC */
 CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // 0x01000000;
 /* Disable clock cycle counter */
 DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk; //~0x00000001;
 /* Enable clock cycle counter */
 DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; //0x00000001;
 /* Reset the clock cycle counter value */
 DWT->CYCCNT = 0;
/* 3 NO OPERATION instructions */
__ASM volatile ("NOP");
__ASM volatile ("NOP");
 __ASM volatile ("NOP");
 /* Check if clock cycle counter has started */
if(DWT->CYCCNT)
{
 return 0; /*clock cycle counter started*/
}
else
 {
 return 1; /*clock cycle counter not started*/
 }
}


void DWT_Delay_ms(volatile uint32_t miliseconds){
	while(miliseconds > 0){
		miliseconds--;
		DWT_Delay_us(1000);
	}
}

volatile uint8_t FatFsCnt;
volatile uint8_t Timer1;
volatile uint8_t Timer2;

// SDTimer_Handler function (same as before)
void SDTimer_Handler(void)
{
    if (Timer1 > 0)
        Timer1--;  // Decrease Timer1 if greater than 0

    if (Timer2 > 0)
        Timer2--;  // Decrease Timer2 if greater than 0
}

volatile uint32_t sysTimingDelay;  // Global variable to keep track of time

/**
 * @brief SysTick interrupt handler
 * This function is called every 1 millisecond when the SysTick interrupt occurs.
 * It decrements the sysTimingDelay variable until it reaches 0.
 */
void SysTick_Handler(void)
{
    // Handling for Sys_DelayMs
    if (sysTimingDelay != 0x00)
    {
        sysTimingDelay--;  // Decrease the delay counter for Sys_DelayMs
    }

    // Handling for FatFsCnt and SDTimer_Handler
    FatFsCnt++;
    if (FatFsCnt >= 10)  // Equivalent to 10ms intervals
    {
        FatFsCnt = 0;
        SDTimer_Handler();  // Decrease Timer1 and Timer2 every 10ms
    }
}

/**
 * @brief Initializes SysTick to generate 1ms ticks.
 * This function sets up the SysTick timer to fire an interrupt every 1 millisecond.
 */
void Sys_DelayInit(void)
{
    // Configure SysTick to generate an interrupt every 1ms based on SystemCoreClock
    if(SysTick_Config(SystemCoreClock / 1000))
	{
		while (1);
	}
}

/**
 * @brief Delays the execution for the given number of milliseconds.
 * The function will block the CPU until the specified time has elapsed.
 *
 * @param milliseconds: Number of milliseconds to delay the execution.
 */
void Sys_DelayMs(uint32_t milliseconds)
{
    sysTimingDelay = milliseconds;  // Set the delay time

    // Wait until the sysTimingDelay reaches 0
    while (sysTimingDelay != 0);
}

// Delay function using Timer2
void Delay_Us(uint32_t Time)
{
    TIM_SetCounter(TIM2, 0);
    while (TIM_GetCounter(TIM2) < Time);
}

void Delay_Ms(uint32_t Time) {
    for (uint32_t i = 0; i < Time; i++) {
        Delay_Us(500); // 1000 µs = 1 ms
    }
}

// Configure TIM2 to use as a microsecond delay timer
void TIM2_Config()
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

    TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1; // Prescale to 1 MHz (1 µs period)
    TIM_TimeBaseInitStructure.TIM_Period = 0xFFFF;    // Maximum period for counting
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;

    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);
    TIM_Cmd(TIM2, ENABLE);
}
