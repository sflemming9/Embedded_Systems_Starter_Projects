#include <ltc263x.h>

// Be sure to provide a 64-bit cycle counter function so that we can properly
// busy-wait between transactions.
extern uint64_t getTotalClockCycles(void);

// LTC263x operations
#define LTC263X_CMD_WRITEN 0x0
#define LTC263X_CMD_UPDATEN 0x1
#define LTC263X_CMD_WRITENUPDATEA 0x2
#define LTC263X_CMD_WRITENUPDATEN 0x3
#define LTC263X_CMD_POWERDOWNN 0x4
#define LTC263X_CMD_POWERDOWNA 0x5
#define LTC263X_CMD_INTREF 0x6
#define LTC263X_CMD_EXTREF 0x7
#define LTC263X_CMD_NOP 0xF

// Initialize the peripherals for this device
void LTC263X_Init(Ltc263xT* dac) {
    // Set up the SPI peripheral
    if(dac->spi.initialized == false)
        SPI_Config(&(dac->spi));
    // Set up our local copy of the output registers
    for(int a = 0; a < dac->numOutputs; a++)
        dac->outputs[a] = 0;
    // Make sure we're not selecting the chip
    SPI_SetCS(&(dac->spi));
}

// Set one DAC output register
void LTC263X_SetOutput(Ltc263xT* dac, uint8_t ch) {
    // Compute the command that we're going to send
    uint8_t command = (LTC263X_CMD_WRITENUPDATEN << 4) | (ch & 0x0F);
    // Lower chip-select
    SPI_ResetCS(&(dac->spi));
    // Send our command
    SPI_Transfer8(&(dac->spi), command);
    // Send the data
    SPI_WriteMulti_Big(&(dac->spi), 1, sizeof(uint16_t), &(dac->outputs[ch]));
    // Raise the chip-select line
    SPI_SetCS(&(dac->spi));
}

// Power down the whole chip
void LTC263X_PowerDownAll(Ltc263xT* dac) {
    // Compute the command that we're going to send
    uint8_t command = (LTC263X_CMD_POWERDOWNA << 4) | 0x0F;
    // Lower chip-select
    SPI_ResetCS(&(dac->spi));
    // Send our command
    SPI_Transfer8(&(dac->spi), command);
    // Raise the chip-select line
    SPI_SetCS(&(dac->spi));
}

// Power down just one channel
void LTC263X_PowerDownChannel(Ltc263xT* dac, uint8_t ch) {
    // Compute the command that we're going to send
    uint8_t command = (LTC263X_CMD_POWERDOWNN << 4) | (ch & 0x0F);
    // Lower chip-select
    SPI_ResetCS(&(dac->spi));
    // Send our command
    SPI_Transfer8(&(dac->spi), command);
    // Raise the chip-select line
    SPI_SetCS(&(dac->spi));
}

// Select a voltage reference
void LTC263X_SelectInternalReference(Ltc263xT* dac, bool useIntRef) {
    // Compute the command that we're going to send
    uint8_t command = (useIntRef ? LTC263X_CMD_INTREF:LTC263X_CMD_EXTREF << 4);
    // Lower chip-select
    SPI_ResetCS(&(dac->spi));
    // Send our command
    SPI_Transfer8(&(dac->spi), command);
    // Raise the chip-select line
    SPI_SetCS(&(dac->spi));
}
