#pragma once

#include <stm32f4xx.h>

// Configures the independent watchdog to underflow with the desired period.
// Returns the actual achieved period in microseconds.
// WARNING: Utilizes TIM5 for the duration of execution.
uint32_t IWDG_Config(uint32_t timeoutMicros);
