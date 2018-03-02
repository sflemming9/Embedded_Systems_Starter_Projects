#pragma once

#include <stdbool.h>
#include <assert.h>

#ifdef STM32F4XX
	#include <stm32f4xx.h>
#elif STM32F30X
	#include <stm32f30x.h>
#elif STM32F2XX
    #include <stm32f2xx.h>
#endif

typedef struct PinT {
   GPIO_TypeDef* port;
   void	(*rccfunc)(uint32_t peripheral, FunctionalState NewState);
   uint16_t pin;
   uint16_t pinsource;
   uint32_t clock;
   uint8_t af;
} Pin;

// Atomic set pin high
void Pin_SetHigh(Pin* pin);

// Atomic set pin low
void Pin_SetLow(Pin* pin);

// Non-atomic toggle pin
void Pin_Toggle(Pin* pin);

// Read the digital value of a pin
bool Pin_ReadValue(Pin* pin);

// Configure a GPIO pin with the given parameters
void Pin_ConfigGpioPin(Pin* pin,
                       GPIOMode_TypeDef mode,
                       GPIOSpeed_TypeDef speed,
                       GPIOOType_TypeDef oType,
                       GPIOPuPd_TypeDef puPd,
                       bool useAf);
