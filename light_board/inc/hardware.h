#include <stdint.h>
#include <stm32f4xx.h>
#include "global.h"
#define DUMMY 0

// Associate lights with lightboard channels. This is different for front
// lightboard and rear lightboard
// Use these constants for lightSet() and lightSetAll():

#ifdef IS_FRONT
  typedef enum
  {
    R_HEADLIGHT = (uint32_t)&(TIM8->CCR1),
    L_HEADLIGHT = (uint32_t)&(TIM3->CCR3),
    L_TURN = (uint32_t)&(TIM3->CCR4),
    R_TURN = (uint32_t)&(TIM8->CCR2),
    L_BRAKE = DUMMY,
    C_BRAKE = DUMMY,
    R_BRAKE = DUMMY
  } LightType_t;
#elif IS_REAR
  typedef enum
  {
    R_HEADLIGHT = DUMMY,
    L_HEADLIGHT = DUMMY,
    L_TURN = (uint32_t)&(TIM3->CCR4),
    R_TURN = (uint32_t)&(TIM8->CCR2),
    L_BRAKE = (uint32_t)&(TIM3->CCR3),
    C_BRAKE = (uint32_t)&(TIM8->CCR3),
    R_BRAKE = (uint32_t)&(TIM8->CCR1)
  } LightType_t;
#endif

// Note: TIM3 only supports 16 bits in CCR register. 
typedef enum
{
  BRIGHTNESS_OFF = 0x0,
  BRIGHTNESS_LOW = 0xF,
  BRIGHTNESS_MED = 0x1FF,
  BRIGHTNESS_HIGH = 0x2FF,
  BRIGHTNESS_MAX = 0xFFFF
} BrightnessType_t;

void configHardware(void);
void lightSet(LightType_t light, BrightnessType_t brightness);
void lightSetAll(BrightnessType_t brightness);