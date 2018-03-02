#include <spi.h>
#include <stdint.h>

// Configure the SPI clock and pins
void SPI_Config(Spi* spi) {
	// Declare initialization structures
    SPI_InitTypeDef SPI_InitStructure;

    // Enable the SPI clock
    (*(spi->rccfunc))(spi->clock, ENABLE);

    // Choose a speed based on our processor
    #ifdef STM32F4XX
        GPIOSpeed_TypeDef GPIO_Fast = GPIO_Speed_100MHz;
    #elif STM32F30X
        GPIOSpeed_TypeDef GPIO_Fast = GPIO_Speed_Level_3;
    #elif STM32F2XX
        GPIOSpeed_TypeDef GPIO_Fast = GPIO_Speed_100MHz;
    #endif

    // Configure the MOSI, MISO, SCLK GPIO lines; connecting to AF
    Pin_ConfigGpioPin(&(spi->mosi), GPIO_Mode_AF, GPIO_Fast,
        GPIO_OType_PP, GPIO_PuPd_NOPULL, true);
    Pin_ConfigGpioPin(&(spi->miso), GPIO_Mode_AF, GPIO_Fast,
        GPIO_OType_OD, GPIO_PuPd_NOPULL, true);
    Pin_ConfigGpioPin(&(spi->sclk), GPIO_Mode_AF, GPIO_Fast,
        GPIO_OType_PP, GPIO_PuPd_NOPULL, true);

    // Configure CS, expecting software twiddling
    Pin_ConfigGpioPin(&(spi->cs), GPIO_Mode_OUT, GPIO_Fast,
        GPIO_OType_PP, GPIO_PuPd_NOPULL, false);

    // Configure SPI peripheral
    SPI_I2S_DeInit(spi->spiPeriph);
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
//    SPI_InitStructure.SPI_DataSize = SPI_DataSize_16b;  // enable 16-bit mode
    SPI_InitStructure.SPI_CPOL = spi->cpol;
    SPI_InitStructure.SPI_CPHA = spi->cpha;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler =
		    SPI_GetDivider(spi->spiPeriph, spi->maxClockSpeed);

    if(spi->word16)
      SPI_InitStructure.SPI_DataSize = SPI_DataSize_16b;
    else
      SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;

    // Take care of the receive case when using 8-bit vs 16-bit words
    #if STM32F30X
    if(spi->word16)
        SPI_RxFIFOThresholdConfig(spi->spiPeriph, SPI_RxFIFOThreshold_HF);
    else
        SPI_RxFIFOThresholdConfig(spi->spiPeriph, SPI_RxFIFOThreshold_QF);
    #endif

    SPI_Init(spi->spiPeriph, &SPI_InitStructure);
    SPI_Cmd(spi->spiPeriph, ENABLE);
    spi->initialized = true;
}

uint8_t read8(Spi *spi, uint8_t reg){
    SPI_ResetCS(spi);
    SPI_Transfer8(spi, reg);
    uint8_t rec = SPI_Transfer8(spi,  0x0);
    SPI_SetCS(spi);
    return rec;
}

void write8(Spi *spi, uint8_t reg, uint8_t value){
    SPI_ResetCS(spi);
    SPI_Transfer8(spi, reg);
    SPI_Transfer8(spi, value);
    SPI_SetCS(spi);
}

// Figure out the clock divider
uint32_t SPI_GetDivider(SPI_TypeDef* periph, uint32_t maxClockSpeed) {
    RCC_ClocksTypeDef RCC_ClocksStatus;
    RCC_GetClocksFreq(&RCC_ClocksStatus);

    // Figure out which clock we're running on
    uint32_t periphClock = RCC_ClocksStatus.PCLK2_Frequency;
    if(periph != SPI1)
        periphClock = RCC_ClocksStatus.PCLK1_Frequency;

    // Compute the minimum legal clock divider
    uint32_t ratio = periphClock / maxClockSpeed;
    for(uint32_t a = 1; a <= 8; a++)
        if(ratio >> a == 0)
            return (a - 1) << 3;

    // Give up and return the maximum divisor, 256
    return 7 << 3;
}

// Wait for SPI "transmit empty"
static inline void SPI_WaitTX(SPI_TypeDef* spi) {
    while(!(spi->SR & SPI_I2S_FLAG_TXE));
}

// Wait for SPI "receive not empty"
static inline void SPI_WaitRX(SPI_TypeDef* spi) {
    while(!(spi->SR & SPI_I2S_FLAG_RXNE));
}

// Wait until SPI "busy" clears
static inline void SPI_WaitBSY(SPI_TypeDef* spi) {
    while((spi->SR & SPI_I2S_FLAG_BSY));
}

// Set the chip select line
void SPI_SetCS(Spi* spi) {
    Pin_SetHigh(&(spi->cs));
}

// Reset the chip select line
void SPI_ResetCS(Spi* spi) {
    Pin_SetLow(&(spi->cs));
}

// Transfer data, blocking until we get a return
// 8-bit version
inline uint8_t SPI_Transfer8(Spi* spi, uint8_t value) {
	*(uint8_t *)(&spi->spiPeriph->DR) = value;
	SPI_WaitTX(spi->spiPeriph);
	SPI_WaitRX(spi->spiPeriph);
    //SPI_WaitBSY(spi->spiPeriph);
	return *(uint8_t *)(&spi->spiPeriph->DR);
}


// Transfer data, blocking until we get a return
// 16-bit version
uint16_t SPI_Transfer16(Spi* spi, uint16_t value) {
	*(uint16_t *)(&spi->spiPeriph->DR) = value;
	SPI_WaitTX(spi->spiPeriph);
	SPI_WaitRX(spi->spiPeriph);
    //SPI_WaitBSY(spi->spiPeriph);
	return *(uint16_t *)(&spi->spiPeriph->DR);
}

// Read a bunch of data (8 bits at a time)
void SPI_ReadMulti(Spi* spi,
                   uint32_t numElems,
                   uint32_t sizeElem,
                   void* target) {
	// Clock in data
	for(uint32_t a = 0; a < numElems; a++)
		for(uint32_t b = 0; b < sizeElem; b++)
            ((uint8_t*)target)[a * sizeElem + b] = SPI_Transfer8(spi, 0);
}

// Write a bunch of data (8 bits at a time)
void SPI_WriteMulti(Spi* spi,
                    uint32_t numElems,
                    uint32_t sizeElem,
                    void* source) {
	// Clock out data
	for(uint32_t a = 0; a < numElems; a++)
		for(uint32_t b = 0; b < sizeElem; b++)
            SPI_Transfer8(spi, ((uint8_t*)source)[a * sizeElem + b]);
}

// Transfer a bunch of data (8 bits at a time)
void SPI_TransferMulti(Spi* spi,
                       uint32_t numElems,
                       uint32_t sizeElem,
                       void* source,
                       void* target) {
	// Clock in data
	for(uint32_t a = 0; a < numElems; a++)
		for(uint32_t b = 0; b < sizeElem; b++)
			((uint8_t*)target)[a * sizeElem + b] = SPI_Transfer8(spi,
                ((uint8_t*)source)[a * sizeElem + b]);
//                SPI_ResetCS(spi);
//                for(int i=0; i<numElems; i++){
//                  printf("%x %x\n", ((uint8_t*)source)[i], ((uint8_t*)target)[i]);
//                }
}

// Read a bunch of data (8 bits at a time)
// Big endian version
void SPI_ReadMulti_Big(Spi* spi,
                       uint32_t numElems,
                       uint32_t sizeElem,
                       void* target) {
	// Clock in data
	for(uint32_t a = 0; a < numElems; a++)
		for(uint32_t b = sizeElem; b > 0; b--)
			((uint8_t*)target)[a * sizeElem + b - 1] =  SPI_Transfer8(spi, 0);
}

// Write a bunch of data (8 bits at a time)
// Big endian version
void SPI_WriteMulti_Big(Spi* spi,
                        uint32_t numElems,
                        uint32_t sizeElem,
                        void* source) {
	// Clock out data
	for(uint32_t a = 0; a < numElems; a++)
		for(uint32_t b = sizeElem; b > 0; b--)
			SPI_Transfer8(spi, ((uint8_t*)source)[a * sizeElem + b - 1]);
}

// Transfer a bunch of data (8 bits at a time)
// Big endian version
void SPI_TransferMulti_Big(Spi* spi,
                           uint32_t numElems,
                           uint32_t sizeElem,
                           void* source,
                           void* target) {
	// Clock in data
	for(uint32_t a = 0; a < numElems; a++)
		for(uint32_t b = sizeElem; b > 0; b--)
			((uint8_t*)target)[a * sizeElem + b - 1] = SPI_Transfer8(spi,
				((uint8_t*)source)[a * sizeElem + b - 1]);
}
