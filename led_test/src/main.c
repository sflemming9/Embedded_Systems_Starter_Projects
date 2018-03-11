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
        .port = GPIOD,                          // the 'D' corresponds to location of the pin - pin used is PD12
        .rccfunc = &(RCC_AHB1PeriphClockCmd),
        .pin = GPIO_Pin_12,                     // User LD4: green LED is a user LED connected to the I/O PD12 of the STM32F407VGT6
        .pinsource = GPIO_PinSource12,          // pin source 12
        .clock = RCC_AHB1Periph_GPIOD,          // GPIOD
        .af = NULL                              // alternate function: because we are only using an LED (not spi or other communication protocol, this can be NULL)
    }
};


void LedTest(void* pvParameters)
{

  /* Configure the LEDs */
  // always need to configure the pin
  Pin_ConfigGpioPin(LED, GPIO_Mode_OUT, GPIO_Speed_2MHz,
            GPIO_OType_PP, GPIO_PuPd_NOPULL, false);
  
  while(true)  {
    
    Pin_SetHigh(LED);           // set the pin to HIGH (turns it on)
    /* Wait one second */
    vTaskDelay(1000);           // measured in milli seconds
    
    Pin_SetLow(LED);            // set the pin to LOW (turns it off)
    vTaskDelay(1000);           // wait 1s until loop continues
  
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
