/*
 * Somewhat over-commented simple CAN usage example/starter code.
 */

#include "bsp.h"
#include <canrouter.h>
#include <stdio.h>
#include <stm32f4xx.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <pin.h>
#include <catalog.h>

uint32_t buttonStateId;


Pin LED1 = {GPIOD, &RCC_AHB1PeriphClockCmd, GPIO_Pin_15, GPIO_PinSource15, RCC_AHB1Periph_GPIOD};
Pin LED2 = {GPIOD, &RCC_AHB1PeriphClockCmd, GPIO_Pin_14, GPIO_PinSource14, RCC_AHB1Periph_GPIOD};
Pin LED3 = {GPIOD, &RCC_AHB1PeriphClockCmd, GPIO_Pin_13, GPIO_PinSource13, RCC_AHB1Periph_GPIOD};
Pin LED4 = {GPIOD, &RCC_AHB1PeriphClockCmd, GPIO_Pin_12, GPIO_PinSource12, RCC_AHB1Periph_GPIOD};
xQueueHandle LuminosCANRxQueue;
xQueueHandle CanTxQueue;
xQueueHandle CatalogRxQueue;
xQueueHandle CatalogTxQueue;


/*
 * Basic task to send a CAN message. Define the message parameters
 * to talk properly with the tritium board.
 */
void SendCANMessage(void* pvParameters)
{
  Pin_ConfigGpioPin(&LED1, GPIO_Mode_OUT, GPIO_Speed_50MHz, GPIO_OType_PP, GPIO_PuPd_UP, false);
  Pin_ConfigGpioPin(&LED2, GPIO_Mode_OUT, GPIO_Speed_50MHz, GPIO_OType_PP, GPIO_PuPd_UP, false);
  Pin_ConfigGpioPin(&LED3, GPIO_Mode_OUT, GPIO_Speed_50MHz, GPIO_OType_PP, GPIO_PuPd_UP, false);
  Pin_ConfigGpioPin(&LED4, GPIO_Mode_OUT, GPIO_Speed_50MHz, GPIO_OType_PP, GPIO_PuPd_UP, false);

  Pin_SetHigh(&LED1);
  Pin_SetHigh(&LED2);
  Pin_SetHigh(&LED3);
  Pin_SetHigh(&LED4);
  
  vTaskDelay(10000); // wait ten seconds
  while(true)
  {
    Pin_SetLow(&LED1);
    // prepare  a new message
    CanTxMsg txMsg;
    txMsg.StdId = 0x0; // 0x8 = off-car "board"; 0x31 = arbitrary value (indicates to turn off the tritium)
    txMsg.ExtId =  buttonStateId; // unused parameter
    txMsg.IDE = CAN_ID_EXT; // using a standard identifier (as opposed to an extended one)
    txMsg.RTR = CAN_RTR_DATA; // not expecting a response
    txMsg.DLC = 8; // length of the data array
    char* data = "DATASENT"; // data array (length 8, as specified above)
    strncpy((void*)(&(txMsg.Data[0])), data, 8);

    CAN_Enqueue(&mainCan, &txMsg, false); // send the message
    vTaskDelay(100); // delay
  }
}


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
  }
}

xTaskHandle LuminosCANRxTaskHandle;

/*
 * Basic task to receive and handle a CAN message. Could be any
 * type of message that properly meets CAN specifications (ie, doesn't
 * fail).
 */
void ReceiveCANMessage(void* pvParameters)
{
  static CanRxMsg rxMsg;
  
  while(true)
  { 
    // Respond to BMS and button board CAN
    portBASE_TYPE res = xQueueReceive(LuminosCANRxQueue, &rxMsg, 1000);
    if((res == pdPASS) && isCat(rxMsg.ExtId))
    {
      printf("message received\n");
    }
  }
} 
 
Catalog cat;

#define CAN_BAUD 500000
#define CAN_QUEUE_LEN 256
/*
 * Set up the BSP and CAN and call simple send and receive tasks.
 */

uint32_t myVar;

//the start up files are different?

int main()
{
  // Startup housekeeping
  //RCC_HSICmd(DISABLE);
  BSP_EnableCycleCounter();
  BSP_SetAllAnalog();
  //BSP_PrintStartupInfo();
   // start CAN1 clock - to use CAN2 on its own, both clocks must be running

  
  //CATALOG
      catInitializeCatalog(&cat, 0x4);

      //catInitEntry(&cat, CAT_TID_UINT32, 0x50, FLAG_WRITABLE, 0, "ButtonState", 1, (void *) &myVar);

      catFinishInit(&cat, "DriverControls");
      
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);
  CAN_Config(&mainCan, CAN_BAUD, CAN_QUEUE_LEN);


  // use the same queue for Catalog and CAN router
  CanTxQueue = mainCan.txQueue;

  // extended id filer
  CAN_InitHwF(&mainCan, 0x0, 0x0, true, 14); // CAN2 filters start at 14

  // standard id filter
  CAN_InitHwF(&mainCan, 0x0, 0x0, false, 15);

  // Subscribe to a type of message ID
// match = ((packetId & mask)==(id))
// Matching packets will be put in to the returned queue.

  LuminosCANRxQueue = xQueueCreate(CAN_RXLEN, sizeof(CanRxMsg));

  // TODO: Make this a set value variable in button board, register callback in DC
  uint32_t buttonStateId = assembleID(0x4, 0x50 , 0x0, CAT_OP_VALUE);
  CAN_Subscribe(&mainCan, buttonStateId, buttonStateId, true, LuminosCANRxQueue);

  // FreeRTOS thread creation; call these functions and set them up to loop forever
 // xTaskCreate(turnOnLEDs, (const char *)("turnOnLEDs"), 512, NULL, 1, NULL);
  //xTaskCreate(SendCANMessage, (const char *)("SendCANMessage"), 512, NULL, 1, NULL);
  xTaskCreate(ReceiveCANMessage, (const char *)("ReceiveCANMessage"), 512, NULL, 1, &LuminosCANRxTaskHandle);
  xTaskCreate(CatalogReceiveTask, (const signed char*)("CatalogReceiveTask"), 512, &cat, 1, NULL);
  xTaskCreate(CatalogAnnounceTask, (const signed char *)("CatalogAnnounceTask"), 512, &cat, 1, NULL);

  // start the scheduler
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
  vTaskStartScheduler();

  // should never get here (placeholder); could be replaced by energy-saving code in the future
  while(true);
}