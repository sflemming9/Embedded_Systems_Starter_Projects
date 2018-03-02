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
    float offsets[3];
    float scaleFactors[3];
	int16_t rawData[3];
    float data[3];
} L3GD20;

// Initialize the peripherals
void L3GD20_Init(L3GD20* gyro);

// Quick existence check to verify that a gyro can communicate
uint32_t L3GD20_WhoAmI(L3GD20* gyro);

// Enable the gyro
void L3GD20_TurnOn(L3GD20* gyro);

// Read new X, Y, Z angular velocities
void L3GD20_UpdateReadings(L3GD20* gyro);

// Turn raw readings into floats scaled to natural units
void L3GD20_ComputeDps(L3GD20* gyro);