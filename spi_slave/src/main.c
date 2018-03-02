/* This board acts as the SPI Slave.
 * You should only need to make changes in dev_spi.c.
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4x7_eth.h"
#include "netconf.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

/* Protobufs */
#include "data.pb.h"
#include "lib_pb.h"

/* ETC */
#include "dev_spi.h"

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{
  // The only task. Should receive packets over SPI
  xTaskCreate(dev_spiTask, (signed const char*)("SPI"), 512, NULL, tskIDLE_PRIORITY + 3, NULL);
  
  /* Start scheduler */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
  vTaskStartScheduler();

  /* We should never get here as control is now taken by the scheduler */
  for( ;; );
}