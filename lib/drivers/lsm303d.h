#pragma once

#ifdef STM32F4XX
	#include <stm32f4xx.h>
#elif STM32F30X
	#include <stm32f30x.h>
#elif STM32F2XX
    #include <stm32f2xx.h>
#endif

#include <stdbool.h>
#include <pin.h>
#include <spi.h>

typedef struct{
    Spi* spi;
    bool initialized;
	int16_t x;
	int16_t y;
	int16_t z;
} L3GD20;

// Initialize the peripherals
void L3GD20_Init(L3GD20* gyro);

// Quick existence check to verify that a gyro can communicate
uint32_t L3GD20_WhoAmI(L3GD20* gyro);

// Enable the gyro
void L3GD20_TurnOn(L3GD20* gyro);

// Read new X, Y, Z angular velocities
void L3GD20_UpdateReadings(L3GD20* gyro);
