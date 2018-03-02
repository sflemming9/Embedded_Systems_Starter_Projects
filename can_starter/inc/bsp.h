#pragma once

#include "canrouter.h"

// ###################
// ##### M4 CORE #####
// ###################

#define CYCCNT (*(volatile const uint32_t*)0xE0001004)

#define DWT_CTRL (*(volatile uint32_t*)0xE0001000)
#define DWT_CTRL_CYCEN 0x00000001

#define SCB_DEMCR (*(volatile uint32_t*)0xE000EDFC)
#define SCB_DEMCR_TRCEN 0x01000000

// ############################
// ##### GLOBAL VARIABLES #####
// ############################

extern volatile uint64_t grossCycleCount;
extern Can mainCan;

// ###################################
// ##### GLOBAL HELPER FUNCTIONS #####
// ###################################

// Compute the total number of elapsed clock cycles since boot
uint64_t BSP_TotalClockCycles(void);

// Compute the number of milliseconds elapsed since boot
uint64_t BSP_TimeMillis(void);

// Set all GPIO banks to analog in and disabled to save power
void BSP_SetAllAnalog(void);

// M3/M4 core configuration to turn on cycle counting since boot
void BSP_EnableCycleCounter(void);

// Print out some clock information
void BSP_PrintStartupInfo();