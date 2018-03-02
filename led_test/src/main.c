// STM32F4 LED test
// IAR toolchain
// 72MHz sysclock w/8MHz crystal

#include "bsp.h"

#include <canrouter.h>
#include <stdio.h>
#include <stm32f4xx.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <pin.h>
#include <catalog.h>

/* Put your LED's here 
* Try looking in the button_board project to see how it turns on LEDs.
* Then look for the pins "names" in the discovery board datasheet. Its on the internal site.
* Hint: PC22 means Port C pin 22
*/

Pin LED[] = {
    {   // Define LED
        .port = GPIOD,
        .rccfunc = &(RCC_AHB1PeriphClockCmd),
        .pin = GPIO_Pin_12,
        .pinsource = GPIO_PinSource12,
        .clock = RCC_AHB1Periph_GPIOD,
        .af = NULL
    }
};


void LedTest(void* pvParameters)
{

  /* Configure the LEDs */
  Pin_ConfigGpioPin(LED, GPIO_Mode_OUT, GPIO_Speed_2MHz,
            GPIO_OType_PP, GPIO_PuPd_NOPULL, false);
  
  while(true)  {
    Pin_SetHigh(LED);
    /* Wait one second */
    vTaskDelay(1000);
    
    Pin_SetLow(LED);
    vTaskDelay(1000);
  
  }
}

int main()
{
  // Startup housekeeping
  RCC_HSICmd(DISABLE);
  BSP_EnableCycleCounter();
  BSP_SetAllAnalog();
  //BSP_PrintStartupInfo();

  // Initialize CAN
  CAN_Config(&can, 500000, 256); // 125kbps, 256 item tx queue
  CAN_InitHwF(&can, 0, 0, true, 0); // HW filter 0 receives all extid packets

  // catInitializeCatalog(&cat, DEVICE_ID);
  
  /*This is were tasks get created
   *they allow us to run several functions virtually in parallel.
   *Right now only Led_Test is running
  */
  //xTaskCreate(CatalogReceiveTask, (const signed char*)("CatalogReceiveTask"), 512, &cat, 1, NULL);
  //xTaskCreate(CatalogAnnounceTask, (const signed char *)("CatalogAnnounceTask"), 512, &cat, 1, NULL);
  xTaskCreate(LedTest, (const char *)("LedTest"), 512, NULL, 1, NULL);

  // Start the scheduler
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
  vTaskStartScheduler();

  // Should never get here
  while(true);
}
