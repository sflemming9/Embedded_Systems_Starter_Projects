/* Make your changes in here!
 */

#include "dev_spi.h"
#include "pin.h"

#include "data.pb.h"
#include "lib_pb.h"

#define PB_SIZE 60
#define MESSAGE_SIZE 20

Spi demo_spi = {
    .spiPeriph = SPI2,
    .clock = RCC_APB1Periph_SPI2,
    .rccfunc = &(RCC_APB1PeriphClockCmd),
    .af = GPIO_AF_SPI2,
    .cs = {
        // TODO: fill in the pin information.
    },
    .sclk = {
        // TODO: fill in
    },
    .miso = {
        // TODO: fill in
    },
    .mosi = {
        // TODO: fill in
    },
    .maxClockSpeed = 1000000,
    .cpol = SPI_CPOL_Low,
    .cpha = SPI_CPHA_1Edge,
    .spi_mode = SPI_Mode_Slave,  // The normal SPI library assumes that our
                                 // boards will always be a master. This project
                                 // uses a custom spi.c file to force this board
                                 // to be the slave.
    .word16 = false,
    .initialized = false
};

void dev_spiTask(void *pvParameters)
{
  // TODO: initialize SPI
  
  char message[MESSAGE_SIZE];
  while(true) {
    // TODO: Receive the message sent by the master
  }
}