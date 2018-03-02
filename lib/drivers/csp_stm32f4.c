#include <csp.h>
#include <limits.h>

// Variables local to this source file
volatile static int64_t CSP_grossCycleCount = 0;

// Registers and flags related to the Cortex-M4F cycle counter
#define CYCCNT (*(volatile const uint32_t*)0xE0001004)
#define DWT_CTRL (*(volatile uint32_t*)0xE0001000)
#define DWT_CTRL_CYCEN 0x00000001
#define SCB_DEMCR (*(volatile uint32_t*)0xE000EDFC)
#define SCB_DEMCR_TRCEN 0x01000000

// Call more often than 2^32 cycles to maintain a 64-bit cycle count
void CSP_UpdateGrossCycleCount(void){
    static uint32_t lastCycleCount = 0;
    uint32_t currentCycleCount = CYCCNT;

    if(currentCycleCount < lastCycleCount){
        CSP_grossCycleCount += ULONG_MAX;
    }

    lastCycleCount = currentCycleCount;
}

// Compute the total number of elapsed clock cycles since boot
int64_t CSP_TotalClockCycles(void){
    // Define the order of volatile accesses by creating a temporary variable
    int64_t bigPart = CSP_grossCycleCount;
    return bigPart + CYCCNT;
}

// Compute the number of milliseconds elapsed since boot
int64_t CSP_TimeMillis(void){
    return CSP_TotalClockCycles() * 1000 / SystemCoreClock;
}

// Return flash size in 32-bit words
int32_t CSP_GetFlashSize(void){
    return ((*(volatile uint32_t*)0x1FFF7A22) & 0x0000FFFF) << 8;
}

// Return flash start address for this processor
int32_t CSP_GetFlashStartAddr(void){
    return 0x08000000;
}

// Reset the CPU
void CSP_Reboot(void){
    NVIC_SystemReset();
}

// Set all GPIO banks to analog in and disabled to save power
void CSP_SetInputsIdle(){
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
void CSP_EnableCycleCounter(){
    // Trace enable, which in turn enables the DWT_CTRL register
    SCB_DEMCR |= SCB_DEMCR_TRCEN;
	// Start the CPU cycle counter (read with CYCCNT)
    DWT_CTRL |= DWT_CTRL_CYCEN;
}

// Print out some clock information
void CSP_PrintStartupInfo(){
    // Get some clock information
    RCC_ClocksTypeDef RCC_ClocksStatus;
    RCC_GetClocksFreq(&RCC_ClocksStatus);

    printf("SYSCLK at %u hertz.\r\n", RCC_ClocksStatus.SYSCLK_Frequency);
    printf("HCLK (AHB) at %u hertz.\r\n", RCC_ClocksStatus.HCLK_Frequency);
    printf("PCLK1 (APB1) at %u hertz.\r\n", RCC_ClocksStatus.PCLK1_Frequency);
    printf("PCLK2 (APB2) at %u hertz.\r\n\r\n", RCC_ClocksStatus.PCLK2_Frequency);
}