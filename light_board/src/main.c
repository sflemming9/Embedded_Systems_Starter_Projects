/*
    STM32F4 starter code
	IAR toolchain

    72MHz sysclock w/8MHz crystal
*/
#include <global.h>
#include "hardware.h"
#include "lights_pwm.h"

// ############################
// ##### GLOBAL VARIABLES #####
// ############################

extern bool RightTurnActive;
extern bool LeftTurnActive;
volatile int64_t time_ms;

xQueueHandle CanTxQueue;
xQueueHandle CanRxQueue;
// Incoming catalog messages
xQueueHandle CatalogRxQueue;

Pin canTxPin;
Pin canRxPin;

Can mainCan = 
{
  .canPeriph = CAN1,
  .clock = RCC_APB1Periph_CAN1,
  .rccfunc = &(RCC_APB1PeriphClockCmd),
  .canTx = {
      .port = GPIOD,
      .pin = GPIO_Pin_1,
      .pinsource = GPIO_PinSource1,
      .clock = RCC_AHB1Periph_GPIOD,
      .rccfunc = &(RCC_AHB1PeriphClockCmd),
      .af = GPIO_AF_CAN1
  },
  .canRx = {
      .port = GPIOD,
      .pin = GPIO_Pin_0,
      .pinsource = GPIO_PinSource0,
      .clock = RCC_AHB1Periph_GPIOD,
      .rccfunc = &(RCC_AHB1PeriphClockCmd),
      .af = GPIO_AF_CAN1
  },
  .initialized = false,
  .nextSubscriber = NULL
};

volatile uint64_t cycleOverflowsTimesMAXINT = 0;
volatile uint64_t grossCycleCount = 0;

//controls the period of the turn signal in milliseconds
#define TURN_PERIOD 500

#define CAN_SPEED 500000
#define CAN_QUEUE_LEN 256


xQueueHandle CanTxQueue;
xQueueHandle CanRxQueue;
//Incoming catalog messages
xQueueHandle CatalogRxQueue;

Catalog cat;

/* Private function prototypes -----------------------------------------------*/
void TIM_Config(void);

// ##########################
// ##### FREERTOS HOOKS #####
// ##########################


// Hook to increment the system timer
void vApplicationTickHook(void)
{
  static uint32_t lastCycleCount = 0;
  uint32_t currentCycleCount = CYCCNT;
  
  if(currentCycleCount < lastCycleCount)
  {
      cycleOverflowsTimesMAXINT += 4294967296;
  }
  
  lastCycleCount = currentCycleCount;

  time_ms++;
}

// Compute the total number of elapsed clock cycles since boot
uint64_t BSP_TotalClockCycles(void)
{
    // Define the order of volatile accesses by creating a temporary variable
    uint64_t bigPart = grossCycleCount;
    return bigPart + CYCCNT;
}

// Compute the number of milliseconds elapsed since boot
uint64_t BSP_TimeMillis(void)
{
	return time_ms;
    //return BSP_TotalClockCycles() * 1000 / SystemCoreClock;
}

// Print out some clock information
void BSP_PrintStartupInfo(){
    // Get some clock information
    RCC_ClocksTypeDef RCC_ClocksStatus;
    RCC_GetClocksFreq(&RCC_ClocksStatus);

    printf("Luminos Lightboard\r\n");
    printf("SYSCLK at %u hertz.\r\n", RCC_ClocksStatus.SYSCLK_Frequency);
    printf("HCLK at %u hertz.\r\n", RCC_ClocksStatus.HCLK_Frequency);
    printf("PCLK1 at %u hertz.\r\n", RCC_ClocksStatus.PCLK1_Frequency);
    printf("PCLK2 at %u hertz.\r\n\r\n", RCC_ClocksStatus.PCLK2_Frequency);
}


// Hook to execute whenever entering the idle task
// This is a good place for power-saving code.
void vApplicationIdleHook(void){}

// Hook to handle stack overflow
void vApplicationStackOverflowHook(xTaskHandle xTask,
                                   signed portCHAR *pcTaskName){}

// ############################
// ##### HELPER FUNCTIONS #####
// ############################

uint64_t getTotalClockCycles()
{
  // Define the order of volatile accesses by creating a temporary variable
  uint64_t bigPart = cycleOverflowsTimesMAXINT;
  return bigPart + CYCCNT;
}

// #################
// ##### TASKS #####
// #################


// ################
// ##### MAIN #####
// ################

int main()
{
  // Trace enable, which in turn enables the DWT_CTRL register
  SCB_DEMCR |= SCB_DEMCR_TRCEN;
  // Start the CPU cycle counter (read with CYCCNT)
  DWT_CTRL |= DWT_CTRL_CYCEN;
  //BSP_PrintStartupInfo();
  // Disable unused pins
  GPIO_SetAllAnalog();

  configHardware();

  
  //configure can
  canTxPin.port = GPIOD;
  canTxPin.pin = GPIO_Pin_1;
  canTxPin.pinsource = GPIO_PinSource1;
  canTxPin.clock = RCC_AHB1Periph_GPIOD;
  canTxPin.rccfunc = &(RCC_AHB1PeriphClockCmd);
  canTxPin.af = GPIO_AF_CAN1;

  canRxPin.port = GPIOD;
  canRxPin.pin = GPIO_Pin_0;
  canRxPin.pinsource = GPIO_PinSource0;
  canRxPin.clock = RCC_AHB1Periph_GPIOD;
  canRxPin.rccfunc = &(RCC_AHB1PeriphClockCmd);
  canRxPin.af = GPIO_AF_CAN1;
  
  //mainCan is a global
  mainCan.canPeriph = CAN1;
  mainCan.clock = RCC_APB1Periph_CAN1;
  mainCan.rccfunc = &(RCC_APB1PeriphClockCmd);
  mainCan.canTx = canTxPin;
  mainCan.canRx = canRxPin;
  mainCan.initialized = false;
  mainCan.nextSubscriber = NULL;
  
  CAN_Config(&mainCan, 500000, 256);
  CAN_InitHwF(&mainCan, 0x0, 0x0, true, 0);
  
  CanRxQueue = xQueueCreate(CAN_RXLEN, sizeof(CanRxMsg));
  CanTxQueue = mainCan.txQueue;
  
  // Get some clock information
  RCC_ClocksTypeDef RCC_ClocksStatus;
  RCC_GetClocksFreq(&RCC_ClocksStatus);
  
  catInitializeCatalog(&cat, DEVICE_ID);
  // Add entries here
  catInitEntry(&cat, CAT_TID_UINT64, 0x20, FLAG_ANNOUNCED | FLAG_TRANSIENT, 500,
               "LBTime", 1, (void*)(&time_ms));
  LightsCatVarInit(&cat);
  catFinishInit(&cat, DEVICE_NAME);
  
  // This has to be after catInitializeCatalog because that's when
  // CatalogRxQueue is initialized
  CAN_Subscribe(&mainCan, CAT_IDMASK_DEVID, DEVICE_ID << CAT_IDSHIFT_DEVID, true, CatalogRxQueue);


  // Set up tasks and start the scheduler
  xTaskCreate(CatalogReceiveTask, (const signed char*)("CatalogReceiveTask"), 1024, &cat, 1, NULL);
  xTaskCreate(CatalogAnnounceTask, (const signed char *)("CatalogAnnounceTask"), 1024, &cat, 1, NULL);

  xTaskCreate(LightsUpdateTask, (const signed char *)("Update lights"), 256, NULL, 1, NULL);
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
  vTaskStartScheduler();
  // Should never get here
  while(1);	   
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  while(1);
}
#endif