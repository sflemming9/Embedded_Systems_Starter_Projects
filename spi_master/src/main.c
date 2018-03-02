/* This board acts as the SPI master.
 * You should only need to make changes in dev_spi.c.
 */

/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>

#include "stm32f4x7_eth.h"
#include "netconf.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "app_imu.h"
#include "dev_adc.h"


/* Protobufs */
#include "data.pb.h"
#include "lib_pb.h"

/* ETC */
#include "dev_spi.h"
#include "app_rcv_data.h"

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{
  // The only task. Should send packets over SPI
  xTaskCreate(dev_spiTask, (signed const char*)("SPI"), 512, NULL, tskIDLE_PRIORITY + 3, NULL);

  /* Start scheduler */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
  vTaskStartScheduler();

  /* We should never get here as control is now taken by the scheduler */
  for( ;; );
}