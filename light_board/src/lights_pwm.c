#include "hardware.h"
#include "lights_pwm.h"

// shared with driver controls
#define RR_LIGHTS_MSG_VID 0xEE
#define RHL_IND 0
#define LHL_IND 1
#define LTURN_IND 2
#define RTURN_IND 3
#define LBRAKE_IND 4
#define CBRAKE_IND 5
#define RBRAKE_IND 6

#define NCHANNELS 7

// Most significant 32-bits describe light index, LSB 32-bits describe brightness
uint32_t light_state;
BrightnessType_t getBrightness(int power);

void setLight(int i, BrightnessType_t brightness)
{
  switch (i)
  {
    
    case RHL_IND:
      lightSet(R_HEADLIGHT, brightness);
      return;
    case LHL_IND:
      lightSet(L_HEADLIGHT, brightness);
      return;
    case LTURN_IND:
      lightSet(L_TURN, brightness);
      return;
    case RTURN_IND:
      lightSet(R_TURN, brightness);
      return;
    case LBRAKE_IND:
      lightSet(L_BRAKE, brightness);
      return;
    case CBRAKE_IND:
      lightSet(C_BRAKE, brightness);
      return;
    case RBRAKE_IND:
      lightSet(R_BRAKE, brightness);
      return;
    default:
      return;
  }
}

void LightsUpdateTask(void *data)
{
  // Lights are all off
  light_state = 0;
  lightSetAll(BRIGHTNESS_OFF);

  //LEDDriversEnable();
  //while(true);
  while(true)
  {
    for(int i = 0; i < NCHANNELS; i++)
    {
      int power = light_state >> (3*i);
      power = power << 29;
      power = power >> 29;
      BrightnessType_t brightness = getBrightness(power);
      setLight(i, brightness);
    }
    vTaskDelay(100);
 }
}

void LightsCatVarInit(Catalog *cat)
{
    catInitEntry(cat, CAT_TID_UINT32, RR_LIGHTS_MSG_VID, FLAG_WRITABLE, 0, "lights brightness", 1, &light_state);
}

BrightnessType_t getBrightness(int power)
{
  switch(power)
  {
    case 0:
      return BRIGHTNESS_OFF;
    case 1:
      return BRIGHTNESS_LOW;
    case 2:
      return BRIGHTNESS_MED;
    case 3:
      return BRIGHTNESS_HIGH;
    case 4:
      return BRIGHTNESS_MAX;
    default:
      return BRIGHTNESS_OFF;
  }
}