#pragma once

#include <stm32f4xx.h>
#include <stdio.h>

// Call more often than 2^32 cycles to maintain a 64-bit cycle count
void CSP_UpdateGrossCycleCount(void);

// Compute the total number of elapsed clock cycles since boot
int64_t CSP_TotalClockCycles(void);

// Compute the number of milliseconds elapsed since boot
int64_t CSP_TimeMillis(void);

// Return flash size in 32-bit words
int32_t CSP_GetFlashSize(void);

// Return flash start address for this processor
int32_t CSP_GetFlashStartAddr(void);

// Reset the CPU
void CSP_Reboot(void);

// Set all GPIO banks to analog in and disabled to save power
void CSP_SetInputsIdle(void);

// M3/M4 core configuration to turn on cycle counting since boot
void CSP_EnableCycleCounter(void);

// Print out some clock information
void CSP_PrintStartupInfo();