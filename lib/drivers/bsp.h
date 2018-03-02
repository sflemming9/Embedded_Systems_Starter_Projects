#pragma once

#include <stm32f4xx.h>

// ###################
// ##### M4 CORE #####
// ###################

#define CYCCNT (*(volatile const uint32_t*)0xE0001004)

#define DWT_CTRL (*(volatile uint32_t*)0xE0001000)
#define DWT_CTRL_CYCEN 0x00000001

#define SCB_DEMCR (*(volatile uint32_t*)0xE000EDFC)
#define SCB_DEMCR_TRCEN 0x01000000

// ##########################
// ##### HELPFUL MACROS #####
// ##########################

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define BOUND(_val, _max, _min)	(MIN(_max, MAX(_val, _min)))

// ############################
// ##### GLOBAL VARIABLES #####
// ############################

extern volatile int64_t time_ms;

// ################################
// ##### BSP HELPER FUNCTIONS #####
// ################################

// Compute the total number of elapsed clock cycles since boot
int64_t BSP_TotalClockCycles(void);

// Compute the number of milliseconds elapsed since boot
int64_t BSP_TimeMillis(void);

// Return flash size in 32-bit words
int32_t BSP_GetFlashSize(void);

// Return flash start address for this processor
int32_t BSP_GetFlashStartAddr(void);

// Reset the CPU
void BSP_Reboot(void);

// Set all GPIO banks to analog in and disabled to save power
void BSP_SetAllAnalog(void);

// M3/M4 core configuration to turn on cycle counting since boot
void BSP_EnableCycleCounter(void);

// Print out some clock information
void BSP_PrintStartupInfo();