#pragma once

// This driver operates standard SPI EEPROM devices on the STM32 series of
// processors. See representative chip:
// http://ww1.microchip.com/downloads/en/devicedoc/22064d.pdf

#include <stdbool.h>
#include <spi.h>
#include <pin.h>

#ifdef STM32F4XX
    #include <stm32f4xx.h>
#elif STM32F30X
    #include <stm32f30x.h>
#elif STM32F2XX
    #include <stm32f2xx.h>
#endif

typedef struct EepromT {
    Spi* spi;
    uint32_t addressBytes;
    uint32_t memoryBytes;
    bool initialized;
} Eeprom;

// Initialize the peripherals for this device
void EEPROM25_Init(Eeprom* eeprom);

// Enable writes
void EEPROM25_WriteEnable(Eeprom* eeprom);

// Disable writes
void EEPROM25_WriteDisable(Eeprom* eeprom);

// Read the status register
uint8_t EEPROM25_ReadStatus(Eeprom* eeprom);

// Transfer an arbitrary chunk of memory
bool EEPROM25_Transfer(Eeprom* eeprom,
                        uint32_t location,
					    void* data,
					    uint32_t len,
                        bool isWrite);

// Fully erase the entire EEPROM
void EEPROM25_Erase(Eeprom* eeprom);

// Check to ensure that the EEPROM is blank
bool EEPROM25_BlankCheck(Eeprom* eeprom);
