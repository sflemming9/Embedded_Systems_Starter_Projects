/* Special copy of SPI, since it is slightly different from the lib spi code.
 * The only change is that the spi_mode is now a field in the spi struct,
 * since this code requires the SPI slave mode. */

#pragma once

#include <stdbool.h>
#include "pin.h"

#ifdef STM32F4XX
	#include <stm32f4xx.h>
#elif STM32F30X
	#include <stm32f30x.h>
#elif STM32F2XX
    #include <stm32f2xx.h>
#endif

typedef struct SpiT {
    SPI_TypeDef *spiPeriph;
    uint32_t clock;
    void (*rccfunc)(uint32_t peripheral, FunctionalState NewState);
    uint8_t af;
    Pin cs;
    Pin sclk;
    Pin miso;
    Pin mosi;
    uint32_t maxClockSpeed;
    uint16_t cpol;
    uint16_t cpha;
    uint16_t spi_mode;
    bool word16;
    bool initialized;
} Spi;

// Configure the SPI clock and pins
void SPI_Config(Spi* spi);

// Figure out the clock divider
uint32_t SPI_GetDivider(SPI_TypeDef* periph, uint32_t maxClockSpeed);

// Set the chip select line
void SPI_SetCS(Spi* spi);

// Reset the chip select line
void SPI_ResetCS(Spi* spi);

// Transfer data, blocking until we get a return
// 8-bit version
inline uint8_t SPI_Transfer8(Spi* spi, uint8_t value);

// Transfer data, blocking until we get a return
// 16-bit version
inline uint16_t SPI_Transfer16(Spi* spi, uint16_t value);

// Read a bunch of data
void SPI_ReadMulti(Spi* spi,
                   uint32_t numElems,
                   uint32_t sizeElem,
                   void* target);

// Write a bunch of data
void SPI_WriteMulti(Spi* spi,
                    uint32_t numElems,
                    uint32_t sizeElem,
                    void* source);

// Transfer a bunch of data
void SPI_TransferMulti(Spi* spi,
                       uint32_t numElems,
                       uint32_t sizeElem,
                       void* source,
                       void* target);

// Read a bunch of data
void SPI_ReadMulti_Big(Spi* spi,
                       uint32_t numElems,
                       uint32_t sizeElem,
                       void* target);

// Write a bunch of data
void SPI_WriteMulti_Big(Spi* spi,
                        uint32_t numElems,
                        uint32_t sizeElem,
                        void* source);

// Transfer a bunch of data
void SPI_TransferMulti_Big(Spi* spi,
                           uint32_t numElems,
                           uint32_t sizeElem,
                           void* source,
                           void* target);
