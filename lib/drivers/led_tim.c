#include <led_tim.h>

static const uint32_t kPERIOD = 10000; // 10 milliseconds per period

// Configure TIM-based LEDs
void TIMLED_Config(TimLed* timLed){
    // Configuration data placeholders
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    RCC_ClocksTypeDef RCC_ClocksStatus;

    // Compute the prescaler and configure the timebase
    RCC_GetClocksFreq(&RCC_ClocksStatus);
    (*(timLed->rccfunc))(timLed->clock, ENABLE);
    uint32_t clockFreq = 0;
    if(IS_RCC_APB1_PERIPH(timLed->clock)){
        clockFreq = RCC_ClocksStatus.PCLK1_Frequency;
		clockFreq = (RCC->CFGR & 0x1000) ? clockFreq << 1 : clockFreq;
    } else if(IS_RCC_APB2_PERIPH(timLed->clock)){
		clockFreq = RCC_ClocksStatus.PCLK2_Frequency;
		clockFreq = (RCC->CFGR & 0x8000) ? clockFreq << 1: clockFreq;
    }
	TIM_TimeBaseStructure.TIM_Prescaler = clockFreq / 1000000;
    TIM_TimeBaseStructure.TIM_Period = kPERIOD - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(timLed->timer, &TIM_TimeBaseStructure);

    // Set LEDs to push-pull output and connect them to the timer outputs
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    for(int a = 0; a < timLed->numLeds; a++){
        Pin_ConfigGpioPin(&(timLed->leds[a].pin), GPIO_Mode_AF,
            GPIO_Speed_2MHz, GPIO_OType_PP, GPIO_PuPd_NOPULL, true);
        volatile uint32_t* ccr = timLed->leds[a].ccr;
        if(ccr == &(timLed->timer->CCR1))
            TIM_OC1Init(timLed->timer, &TIM_OCInitStructure);
        else if(ccr == &(timLed->timer->CCR2))
            TIM_OC2Init(timLed->timer, &TIM_OCInitStructure);
        else if(ccr == &(timLed->timer->CCR3))
            TIM_OC3Init(timLed->timer, &TIM_OCInitStructure);
        else if(ccr == &(timLed->timer->CCR4))
            TIM_OC4Init(timLed->timer, &TIM_OCInitStructure);
        timLed->leds[a].flashOnce = true;
    }

    // Enable the update interrupt
    NVIC_InitStructure.NVIC_IRQChannel = timLed->irq;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = timLed->irqPriority;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // Start the timer
    TIM_ITConfig(timLed->timer, TIM_IT_Update, ENABLE);
    TIM_Cmd(timLed->timer, ENABLE);
}

// Flash a chosen LED for a given duration
void TIMLED_Flash(TimLed* timLed, uint32_t led, uint32_t durationMillis){
    *(timLed->leds[led].ccr) = (timLed->timer->CNT + (durationMillis * 1000));
    timLed->leds[led].flashOnce = true;
}

// Call this from the timer update interrupt
void TIMLED_Isr(TimLed* timLed){
    TIM_ClearITPendingBit(timLed->timer, TIM_IT_Update);
    for(int a = 0; a < timLed->numLeds; a++){
        // Let the PWM peripheral do its thing unless blinking once
        if(!(timLed->leds[a].flashOnce))
            continue;

        // Update one-blink LEDs
        volatile uint32_t* ccr = timLed->leds[a].ccr;
        if(*ccr > kPERIOD){
            *ccr -= kPERIOD;
        } else {
            *ccr = 0;
        }
    }
}

// Choose whether to duty cycle control or blink once
void TIMLED_SetFlashOnce(TimLed* timLed, uint32_t led, bool flashOnce){
    timLed->leds[led].flashOnce = flashOnce;
}

// Set the duty cycle of an LED output
void TIMLED_SetDutyCycle(TimLed* timLed, uint32_t led, float dutyCycle){
    *(timLed->leds[led].ccr) = (uint32_t)(dutyCycle * kPERIOD);
}
