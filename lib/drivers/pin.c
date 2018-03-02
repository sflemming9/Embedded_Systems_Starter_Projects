#include <pin.h>

// Atomic set pin high
void Pin_SetHigh(Pin* pin) {
	#ifdef STM32F4XX
		pin->port->BSRRL = pin->pin;
	#elif STM32F30X
		pin->port->BSRR = pin->pin;
	#endif
}

// Atomic set pin low
void Pin_SetLow(Pin* pin) {
	#ifdef STM32F4XX
		pin->port->BSRRH = pin->pin;
	#elif STM32F30X
		pin->port->BRR = pin->pin;
	#endif
}

// Non-atomic toggle pin
void Pin_Toggle(Pin* pin) {
    if(pin->port->ODR & pin->pin)
        Pin_SetLow(pin);
    else
        Pin_SetHigh(pin);
}

// Read the digital value of a pin
bool Pin_ReadValue(Pin* pin) {
    return (pin->port->IDR & pin->pin) ? true : false;
}

// Configure a GPIO pin with the given parameters
void Pin_ConfigGpioPin(Pin* pin,
                       GPIOMode_TypeDef mode,
                       GPIOSpeed_TypeDef speed,
                       GPIOOType_TypeDef oType,
                       GPIOPuPd_TypeDef puPd,
                       bool useAf) {
	assert(pin->rccfunc != 0);
    (*(pin->rccfunc))(pin->clock, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = pin->pin;
    GPIO_InitStructure.GPIO_Mode = mode;
    GPIO_InitStructure.GPIO_Speed = speed;
    GPIO_InitStructure.GPIO_OType = oType;
    GPIO_InitStructure.GPIO_PuPd = puPd;
    GPIO_Init(pin->port, &GPIO_InitStructure);

	if(useAf == true)
		GPIO_PinAFConfig(pin->port, pin->pinsource, pin->af);
}