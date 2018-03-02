#include <pin.h>
#include "hardware.h"

#define PWM_FREQ 20000
#define PWM_PERIOD ((uint32_t)(SystemCoreClock/PWM_FREQ))
#define NUM_SHUTDOWN_PINS 3
#define NUM_LIGHTS 6

void GPIOConfig(void);
void LEDDriversEnable(void);
void LEDDriversDisable(void);
void PWMConfig(void);

// Each light board has 3 LED drivers that each have a shutdown pin
Pin shutdown_pins[NUM_SHUTDOWN_PINS] = 
{
  {
    .port = GPIOC,
    .rccfunc = &(RCC_AHB1PeriphClockCmd),
    .pin = GPIO_Pin_10,
    .pinsource = GPIO_PinSource10,
    .clock = RCC_AHB1Periph_GPIOC,
  },
  {
    .port = GPIOC,
    .rccfunc = &(RCC_AHB1PeriphClockCmd),
    .pin = GPIO_Pin_11,
    .pinsource = GPIO_PinSource11,
    .clock = RCC_AHB1Periph_GPIOC,
  },
  {
    .port = GPIOC,
    .rccfunc = &(RCC_AHB1PeriphClockCmd),
    .pin = GPIO_Pin_12,
    .pinsource = GPIO_PinSource12,
    .clock = RCC_AHB1Periph_GPIOC,
  }
};

// Each light board can drive 6 channels of LEDs.
Pin lights[NUM_LIGHTS] = 
{
  {
    .port = GPIOC,
    .rccfunc = &(RCC_AHB1PeriphClockCmd),
    .pin = GPIO_Pin_6,
    .pinsource = GPIO_PinSource6,
    .clock = RCC_AHB1Periph_GPIOC,
    .af = GPIO_AF_TIM8
  },
  {
    .port = GPIOC,
    .rccfunc = &(RCC_AHB1PeriphClockCmd),
    .pin = GPIO_Pin_7,
    .pinsource = GPIO_PinSource7,
    .clock = RCC_AHB1Periph_GPIOC,
    .af = GPIO_AF_TIM8  
  },
  {
    .port = GPIOC,
    .rccfunc = &(RCC_AHB1PeriphClockCmd),
    .pin = GPIO_Pin_8,
    .pinsource = GPIO_PinSource8,
    .clock = RCC_AHB1Periph_GPIOC,
    .af = GPIO_AF_TIM8
  },
  {
    .port = GPIOC,
    .rccfunc = &(RCC_AHB1PeriphClockCmd),
    .pin = GPIO_Pin_9,
    .pinsource = GPIO_PinSource9,
    .clock = RCC_AHB1Periph_GPIOC,
    .af = GPIO_AF_TIM8
  },
  {
    .port = GPIOB,
    .rccfunc = &(RCC_AHB1PeriphClockCmd),
    .pin = GPIO_Pin_0,
    .pinsource = GPIO_PinSource0,
    .clock = RCC_AHB1Periph_GPIOB,
    .af = GPIO_AF_TIM3
  },
  {
    .port = GPIOB,
    .rccfunc = &(RCC_AHB1PeriphClockCmd),
    .pin = GPIO_Pin_1,
    .pinsource = GPIO_PinSource1,
    .clock = RCC_AHB1Periph_GPIOB,
    .af = GPIO_AF_TIM3
  }
};

void configHardware(void)
{
  GPIOConfig();
  LEDDriversDisable();
  PWMConfig();
}

void GPIOConfig(void)
{
  // Configure light pins
  for(int i = 0; i < NUM_LIGHTS; i++)
  {
    Pin_ConfigGpioPin(&(lights[i]), GPIO_Mode_AF, GPIO_Speed_2MHz, 
                      GPIO_OType_PP, GPIO_PuPd_UP, true);
  }
  // Configure shutdown pins
  for(int i = 0; i < NUM_SHUTDOWN_PINS; i++)
  {
    Pin_ConfigGpioPin(&(shutdown_pins[i]), GPIO_Mode_OUT, GPIO_Speed_2MHz, 
                      GPIO_OType_PP, GPIO_PuPd_UP, true);
  }
}

void LEDDriversEnable(void)
{
  // Set the shutdown pins such that the LED drivers are on.
  GPIO_ResetBits(GPIOC, GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12);
  GPIO_SetBits(GPIOC, GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12);
}

void LEDDriversDisable(void)
{
  GPIO_ResetBits(GPIOC, GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12);
}

void PWMConfig(void)
{ 
  //we're using clock TIM3 (if it's the front light board) and TIM8. Note that
  // TIM8 is a special clock and will need additional configuaration
  
  // Configuration structures
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
  TIM_OCInitTypeDef  TIM_OCInitStructure;
    
  // Enable timer peripheral clocks
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM8, ENABLE);
  
  // Configure the TIM8 timebase
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseStructure.TIM_Period = PWM_PERIOD;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
  TIM_TimeBaseInit(TIM8, &TIM_TimeBaseStructure);
  
  // Configure OC[1:4] for PWM output
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;
  TIM_OCInitStructure.TIM_Pulse = 0;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
  TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
  TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;
  TIM_OC3Init(TIM3, &TIM_OCInitStructure);
  TIM_OC4Init(TIM3, &TIM_OCInitStructure);
  TIM_OC1Init(TIM8, &TIM_OCInitStructure);
  TIM_OC2Init(TIM8, &TIM_OCInitStructure);
  TIM_OC3Init(TIM8, &TIM_OCInitStructure);
  TIM_OC4Init(TIM8, &TIM_OCInitStructure);
  
  TIM_Cmd(TIM3, ENABLE);
  TIM_Cmd(TIM8, ENABLE);
  TIM_CtrlPWMOutputs(TIM8, ENABLE);
}

void lightSet(LightType_t light, BrightnessType_t brightness)
{
  if(light == DUMMY)
    return;
      
  *((uint32_t*)light) = brightness;
  
  // Turn off LED drivers when lights are not being set to save some power
  if((TIM8->CCR1 == BRIGHTNESS_OFF) &&
     (TIM8->CCR2 == BRIGHTNESS_OFF) &&
     (TIM8->CCR3 == BRIGHTNESS_OFF) &&
     (TIM8->CCR4 == BRIGHTNESS_OFF) &&
     (TIM3->CCR3 == BRIGHTNESS_OFF) &&
     (TIM3->CCR4 == BRIGHTNESS_OFF))
    LEDDriversDisable();
  else
    LEDDriversEnable();
}

void lightSetAll(BrightnessType_t brightness)
{
  TIM8->CCR1 = brightness;
  TIM8->CCR2 = brightness;
  TIM8->CCR3 = brightness;
  TIM8->CCR4 = brightness;
  TIM3->CCR3 = brightness;
  TIM3->CCR4 = brightness;  
}
