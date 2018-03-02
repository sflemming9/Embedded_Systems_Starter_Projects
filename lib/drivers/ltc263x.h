#pragma once

#include <stdbool.h>
#include <spi.h>
#include <pin.h>

#ifdef STM32F4XX
    #include <stm32f4xx.h>
#elif STM32F30X
    #include <stm32f30x.h>
#elif STM32F2XX
    #include <stm32f2xx.h>
#endif

typedef struct {
    Spi spi;
    uint8_t numOutputs;
    uint16_t outputs[8];
} Ltc263xT;

// Initialize the peripherals for this device
void LTC263X_Init(Ltc263xT* dac);

// Set one DAC register, push the update
void LTC263X_SetOutput(Ltc263xT* dac, uint8_t ch);

// Power down the whole chip
void LTC263X_PowerDownAll(Ltc263xT* dac);

// Power down just one channel
void LTC263X_PowerDownChannel(Ltc263xT* dac, uint8_t ch);

// Select a voltage reference
void LTC263X_SelectInternalReference(Ltc263xT* dac, bool useIntRef);
