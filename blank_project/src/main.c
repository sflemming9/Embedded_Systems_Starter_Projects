/*
 * Somewhat over-commented simple CAN usage example/starter code.
 */

#include "bsp.h"
#include <stdio.h>
#include <stm32f4xx.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <pin.h>


Pin LED1 = {GPIOD, &RCC_AHB1PeriphClockCmd, GPIO_Pin_15, GPIO_PinSource15, RCC_AHB1Periph_GPIOD};
Pin LED2 = {GPIOD, &RCC_AHB1PeriphClockCmd, GPIO_Pin_14, GPIO_PinSource14, RCC_AHB1Periph_GPIOD};
Pin LED3 = {GPIOD, &RCC_AHB1PeriphClockCmd, GPIO_Pin_13, GPIO_PinSource13, RCC_AHB1Periph_GPIOD};
Pin LED4 = {GPIOD, &RCC_AHB1PeriphClockCmd, GPIO_Pin_12, GPIO_PinSource12, RCC_AHB1Periph_GPIOD};

void turnOnLEDs(void* pvParameters)
{
  Pin_ConfigGpioPin(&LED1, GPIO_Mode_OUT, GPIO_Speed_50MHz, GPIO_OType_PP, GPIO_PuPd_UP, false);
  Pin_ConfigGpioPin(&LED2, GPIO_Mode_OUT, GPIO_Speed_50MHz, GPIO_OType_PP, GPIO_PuPd_UP, false);
  Pin_ConfigGpioPin(&LED3, GPIO_Mode_OUT, GPIO_Speed_50MHz, GPIO_OType_PP, GPIO_PuPd_UP, false);
  Pin_ConfigGpioPin(&LED4, GPIO_Mode_OUT, GPIO_Speed_50MHz, GPIO_OType_PP, GPIO_PuPd_UP, false);

  while(true) {
    Pin_SetHigh(&LED1);
    Pin_SetHigh(&LED2);
    Pin_SetHigh(&LED3);
    Pin_SetHigh(&LED4);
    printf("Heartbeat\n");
    vTaskDelay(1000);
  }
}

int main()
{

  BSP_EnableCycleCounter();
  BSP_SetAllAnalog();
      
  xTaskCreate(turnOnLEDs, (const signed char*)("Blink"), 256, NULL, tskIDLE_PRIORITY + 1, NULL);

  // start the scheduler
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
  vTaskStartScheduler();

  // should never get here (placeholder); could be replaced by energy-saving code in the future
  while(true);
}