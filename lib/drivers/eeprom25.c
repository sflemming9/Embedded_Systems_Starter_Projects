#include <eeprom25.h>
#include <bsp.h>

// EEPROM25 operations
#define EEPROM_READ 0x03
#define EEPROM_WRITE 0x02
#define EEPROM_WRDI 0x04
#define EEPROM_WREN 0x06
#define EEPROM_RDSR 0x05
#define EEPROM_WRSR 0x01

// EEPROM25 flags
#define EEPROM_FLAG_WIP 0x01
#define EEPROM_FLAG_WEL 0x02
#define EEPROM_FLAG_BP0 0x04
#define EEPROM_FLAG_BP1 0x08
#define EEPROM_FLAG_WPEN 0x80

// Enforce 50ns minimum delay between CS rising and falling again
static inline void EEPROM25_CsDelay(void) {
    uint64_t target = BSP_TotalClockCycles() +
        (uint64_t)(SystemCoreClock * 50e-9);
    while(BSP_TotalClockCycles() < target);
}

// Initialize the peripherals for this device
void EEPROM25_Init(Eeprom* eeprom) {
    Spi* spi = eeprom->spi;
	// Set up the SPI peripheral
    if(spi->initialized == false)
        SPI_Config(spi);
    // Set the initial state of the CS line
    SPI_SetCS(spi);
    eeprom->initialized = true;
}

// Enable writes
void EEPROM25_WriteEnable(Eeprom* eeprom){
    Spi* spi = eeprom->spi;
    // Lower chip-select
    EEPROM25_CsDelay();
	SPI_ResetCS(spi);
    // Send the write-enable command
	SPI_Transfer8(spi, EEPROM_WREN);
    // Raise the chip-select line
    SPI_SetCS(spi);
}

// Disable writes
void EEPROM25_WriteDisable(Eeprom* eeprom){
    Spi* spi = eeprom->spi;
    // Lower chip-select
    EEPROM25_CsDelay();
	SPI_ResetCS(spi);
    // Send the write-enable command
	SPI_Transfer8(spi, EEPROM_WRDI);
    // Raise the chip-select line
    SPI_SetCS(spi);
}

// Determine if a write is complete
uint8_t EEPROM25_ReadStatus(Eeprom* eeprom){
    Spi* spi = eeprom->spi;
    // Lower chip-select
    EEPROM25_CsDelay();
	SPI_ResetCS(spi);
    // Set the address pointer to the status register
	SPI_Transfer8(spi, EEPROM_RDSR);
    // Read the status register
    uint8_t status = SPI_Transfer8(spi, 0);
    // Raise the chip-select line
    SPI_SetCS(spi);
    // Return the result
    return status;
}

// Transfer an arbitrary chunk of memory
// Note that we must suffer with 64-byte pages, so we interrupt writes on
// boundaries to let it flush.
bool EEPROM25_Transfer(Eeprom* eeprom,
                        uint32_t location,
                        void* data,
                        uint32_t len,
                        bool isWrite){
    // Bail out on a number of silly inputs
    if(data == 0 || len == 0)
        return false;

    Spi* spi = eeprom->spi;

    // Keep track of how many bytes we've moved
    uint32_t bytesMoved = 0;

    // Check device status and make sure we're not about to stomp on a write
    while((EEPROM25_ReadStatus(eeprom) & EEPROM_FLAG_WIP) != 0);
    // Enable writes if we're trying to write
    if(isWrite) {
        EEPROM25_CsDelay();
        EEPROM25_WriteEnable(eeprom);
    }

    // Lower chip-select
    EEPROM25_CsDelay();
	SPI_ResetCS(spi);
    // Send the 'read' or 'write' instruction
    SPI_Transfer8(spi, (isWrite ? EEPROM_WRITE : EEPROM_READ));
    // Send the address
    SPI_WriteMulti_Big(spi, 1, eeprom->addressBytes, &location);
    // Exchange the first byte and make note of the exchange
    ((uint8_t*)data)[bytesMoved] = SPI_Transfer8(spi,
        ((uint8_t*)data)[bytesMoved]);
    bytesMoved++;
    location++;

    // Move the rest of the bytes
    while(bytesMoved < len) {
        // Mind page boundaries
        if(location % 64 == 0) {
            SPI_SetCS(spi);
            EEPROM25_CsDelay();
            if(isWrite) {
                while((EEPROM25_ReadStatus(eeprom) && EEPROM_FLAG_WIP) != 0);
                // We have to re-enable writes too...
                EEPROM25_WriteEnable(eeprom);
            }
            EEPROM25_CsDelay();
            SPI_ResetCS(spi);
            SPI_Transfer8(spi, (isWrite ? EEPROM_WRITE : EEPROM_READ));
            SPI_WriteMulti_Big(spi, 1, eeprom->addressBytes, &location);
        }
        ((uint8_t*)data)[bytesMoved] = SPI_Transfer8(spi,
            ((uint8_t*)data)[bytesMoved]);
        bytesMoved++;
        location++;
    }

    // Raise the chip-select line
	SPI_SetCS(spi);

    // Indicate likely success
    return true;
}

// Fully erase the entire EEPROM
void EEPROM25_Erase(Eeprom* eeprom) {
    // Create some blank data to copy in to the memory
    uint8_t zeros[64];
    for(int a = 0; a < 64; a++)
        zeros[a] = 0;

    // Copy in the data, page-by-page
    for(int a = 0; a < (eeprom->memoryBytes / 64); a++) {
        for(int b = 0; b < 64; b++) {
            zeros[b] = 0;
        }
        EEPROM25_Transfer(eeprom, a * 64, zeros, 64, true);
    }
}

// Check to ensure that the EEPROM is blank
bool EEPROM25_BlankCheck(Eeprom* eeprom) {
    uint8_t data[64];

    // Copy in the data, page-by-page
    for(int a = 0; a < (eeprom->memoryBytes / 64); a++) {
        EEPROM25_Transfer(eeprom, a * 64, data, 64, false);
        for(int b = 0; b < 64; b++) {
            if(data[b] != 0)
                return false;
        }
    }

    return true;
}
