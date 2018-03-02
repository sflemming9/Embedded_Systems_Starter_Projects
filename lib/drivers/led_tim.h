#pragma once

#include <pin.h>

typedef struct LedT {
    volatile uint32_t* ccr;
    Pin pin;
    bool flashOnce;
} Led;

typedef struct TimLedT {
    TIM_TypeDef* timer;
    void (*rccfunc)(uint32_t peripheral, FunctionalState NewState);
    IRQn_Type irq;
    uint32_t irqPriority;
    uint32_t clock;
    uint32_t numLeds;
    Led* leds;
} TimLed;

// Configure TIM-based LEDs
void TIMLED_Config(TimLed* timLed);

// Flash a chosen LED for a given duration
void TIMLED_Flash(TimLed* timLed, uint32_t led, uint32_t durationMillis);

// Call this from the timer update interrupt to turn off LEDs
// Warning: This has some execution time; very short unwanted pulses can happen
void TIMLED_Isr(TimLed* timLed);

// Choose whether to duty cycle control or blink once
void TIMLED_SetFlashOnce(TimLed* timLed, uint32_t led, bool flashOnce);

// Set the duty cycle of an LED output
void TIMLED_SetDutyCycle(TimLed* timLed, uint32_t led, float dutyCycle);
