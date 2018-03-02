#include "bsp.h"

#include <stm32f4xx.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <stdio.h>
#include <pin.h>
#include <limits.h>

Can can = {
    .canPeriph = CAN1,
    .clock = RCC_APB1Periph_CAN1,
    .rccfunc = &(RCC_APB1PeriphClockCmd),
    .canTx = {
        .port = GPIOA,
        .rccfunc = &(RCC_AHB1PeriphClockCmd),
        .pin = GPIO_Pin_12,
        .pinsource = GPIO_PinSource12,
        .clock = RCC_AHB1Periph_GPIOA,
        .af = GPIO_AF_CAN1
    },
    .canRx = {
        .port = GPIOA,
        .rccfunc = &(RCC_AHB1PeriphClockCmd),
        .pin = GPIO_Pin_11,
        .pinsource = GPIO_PinSource11,
        .clock = RCC_AHB1Periph_GPIOA,
        .af = GPIO_AF_CAN1
    },
    .initialized = false
};

volatile uint64_t grossCycleCount = 0;

// ##########################
// ##### FREERTOS HOOKS #####
// ##########################

// Hook to increment the system timer
void vApplicationTickHook(void){
    static uint32_t lastCycleCount = 0;
    uint32_t currentCycleCount = CYCCNT;

    if(currentCycleCount < lastCycleCount){
        grossCycleCount += ULONG_MAX;
    }

    lastCycleCount = currentCycleCount;
}

// Hook to execute whenever entering the idle task
// This is a good place for power-saving code.
void vApplicationIdleHook(void){}

// Hook to handle malloc errors
void vApplicationMallocFailedHook(void){
    while(true){}
}

// Hook to handle stack overflow
void vApplicationStackOverflowHook(xTaskHandle xTask,
                                   signed portCHAR *pcTaskName){}

// Compute the total number of elapsed clock cycles since boot
uint64_t BSP_TotalClockCycles(void){
    // Define the order of volatile accesses by creating a temporary variable
    uint64_t bigPart = grossCycleCount;
    return bigPart + CYCCNT;
}

// Compute the number of milliseconds elapsed since boot
uint64_t BSP_TimeMillis(void){
    return BSP_TotalClockCycles() * 1000 / SystemCoreClock;
}

// Set all GPIO banks to analog in and disabled to save power
void BSP_SetAllAnalog()
{
    // Data structure to represent GPIO configuration information
    GPIO_InitTypeDef GPIO_InitStructure;

    // Enable the peripheral clocks on all GPIO banks
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB |
        RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE |
        RCC_AHB1Periph_GPIOF | RCC_AHB1Periph_GPIOG | RCC_AHB1Periph_GPIOH |
        RCC_AHB1Periph_GPIOI, ENABLE);

    // Set all GPIO banks to analog inputs
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_Init(GPIOE, &GPIO_InitStructure);
    GPIO_Init(GPIOF, &GPIO_InitStructure);
    GPIO_Init(GPIOG, &GPIO_InitStructure);
    GPIO_Init(GPIOH, &GPIO_InitStructure);
    GPIO_Init(GPIOI, &GPIO_InitStructure);

    // Do these separately so we don't break jtag
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All & ~GPIO_Pin_13 & ~GPIO_Pin_14 &
            ~GPIO_Pin_15;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All & ~GPIO_Pin_3 & ~GPIO_Pin_4;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // Disable the clocks to save power
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB |
        RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOE |
        RCC_AHB1Periph_GPIOF | RCC_AHB1Periph_GPIOG | RCC_AHB1Periph_GPIOH |
        RCC_AHB1Periph_GPIOI, DISABLE);
}

// M3/M4 core configuration to turn on cycle counting since boot
void BSP_EnableCycleCounter(){
    // Trace enable, which in turn enables the DWT_CTRL register
    SCB_DEMCR |= SCB_DEMCR_TRCEN;
	// Start the CPU cycle counter (read with CYCCNT)
    DWT_CTRL |= DWT_CTRL_CYCEN;
}

// Print out some clock information
void BSP_PrintStartupInfo(){
    // Get some clock information
    RCC_ClocksTypeDef RCC_ClocksStatus;
    RCC_GetClocksFreq(&RCC_ClocksStatus);

    printf("STM32F4 starter code\r\n");
    printf("SYSCLK at %u hertz.\r\n", RCC_ClocksStatus.SYSCLK_Frequency);
    printf("HCLK at %u hertz.\r\n", RCC_ClocksStatus.HCLK_Frequency);
    printf("PCLK1 at %u hertz.\r\n", RCC_ClocksStatus.PCLK1_Frequency);
    printf("PCLK2 at %u hertz.\r\n\r\n", RCC_ClocksStatus.PCLK2_Frequency);
}
