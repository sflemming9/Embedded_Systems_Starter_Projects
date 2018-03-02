#include <iwdg.h>

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define BOUND(_val, _max, _min)	(MIN(_max, MAX(_val, _min)))

// Configures the independent watchdog to underflow with the desired period.
// Returns the actual achieved period in microseconds.
// WARNING: Utilizes TIM5 for the duration of execution.
uint32_t IWDG_Config(uint32_t timeoutMicros){
    // Configuration data placeholders
    TIM_ICInitTypeDef TIM_ICInitStructure;
    RCC_ClocksTypeDef RCC_ClocksStatus;

    // Enable and wait for the startup of the low speed internal oscillator
    RCC_LSICmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);

	// Figure out the TIM5 clock frequency
    RCC_GetClocksFreq(&RCC_ClocksStatus);
    uint32_t timClockFreq = RCC_ClocksStatus.PCLK1_Frequency;
    timClockFreq = (RCC->CFGR & 0x1000) ? timClockFreq << 1 : timClockFreq;

    // Configure TIM5 to measure the LSI clock speed
    // TIM5 is special; it's the only timer with an LSI input
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
    TIM_RemapConfig(TIM5, TIM5_LSI);
    TIM_PrescalerConfig(TIM5, 0, TIM_PSCReloadMode_Immediate);
    TIM_ICInitStructure.TIM_Channel = TIM_Channel_4;
    TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
    TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV8;
    TIM_ICInitStructure.TIM_ICFilter = 0;
    TIM_ICInit(TIM5, &TIM_ICInitStructure);
    TIM_Cmd(TIM5, ENABLE);

    // Clear the TIM5 status register and wait until we get 8 ticks
    TIM5->SR = 0;
    while(TIM_GetFlagStatus(TIM5, TIM_FLAG_CC4) == RESET);
    // Do it again; we do this twice to ensure we get the full duration
    TIM5->SR = 0;
    while(TIM_GetFlagStatus(TIM5, TIM_FLAG_CC4) == RESET);
    // Compute the LSI frequency
    uint64_t lsiFreq = 8 * timClockFreq / TIM5->CCR4;
    // Compute the IWDG divisor and the reload value
    uint64_t lsiDivisor = (lsiFreq / 0xFFF) + 1;
    lsiDivisor = MIN(lsiDivisor, 256);
    uint64_t iwdgReload = (lsiFreq * timeoutMicros / 1000000) / lsiDivisor;
    iwdgReload = MIN(iwdgReload, 0xFFF);
    uint64_t actualTimeout = 1000000 * lsiDivisor * iwdgReload / lsiFreq;
    // Free up the timer for other uses
    TIM_DeInit(TIM5);

    // Configure the IWDG peripheral for ~1 second nominal max timeout
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    IWDG_SetPrescaler(lsiDivisor >> 3);
    IWDG_SetReload(iwdgReload);

    // Prevent the IWDG counter from decrementing while in a breakpoint
    DBGMCU_APB1PeriphConfig(DBGMCU_IWDG_STOP, ENABLE);

    // Fire up the counter and return the actual time to kill
    IWDG_ReloadCounter();
    IWDG_Enable();
    return (uint32_t)actualTimeout;
}
