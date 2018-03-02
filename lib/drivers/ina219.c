#include <ina219.h>

// Fudge factors
#define INA219_TIMEOUT SystemCoreClock/100

// I2C event timeout
static uint8_t INA219_EventTimeout(I2C_TypeDef* periph, uint32_t event,
	uint32_t timeout, int state)
{
    while(I2C_CheckEvent(periph, event) == state)
    {
        if((timeout--) == 0)
            return INA219_FAILURE;
    }

    return INA219_SUCCESS;
}

// I2C flag timeout
static uint8_t INA219_FlagTimeout(I2C_TypeDef* periph, uint32_t flag,
                                  uint32_t timeout, int state)
{
    while(I2C_GetFlagStatus(periph, flag) == state)
    {
        if((timeout--) == 0)
            return INA219_FAILURE;
    }

    return INA219_SUCCESS;
}

// Deinitialize the sensor
void INA219_DeInit(INA219* sensor)
{
	I2C_Cmd(sensor->peripheral, DISABLE);
	I2C_DeInit(sensor->peripheral);

	(*(sensor->rccfunc))(sensor->clock, DISABLE);
}

// Initialize the sensor
void INA219_Init(INA219* sensor){
    // Initialization structures
    GPIO_InitTypeDef GPIO_InitStructure;
    I2C_InitTypeDef I2C_InitStructure;

    // Enable the I2C clock
    RCC_APB1PeriphClockCmd(sensor->clock, ENABLE);

    // Enable the GPIO clocks
    RCC_AHB1PeriphClockCmd(sensor->sda.clock | sensor->scl.clock, ENABLE);

    // Reset the I2C peripheral
	RCC_APB1PeriphResetCmd(sensor->clock, ENABLE);
    RCC_APB1PeriphResetCmd(sensor->clock, DISABLE);

    // Connect the GPIO pins to the I2C peripheral (alternate function, AF)
    GPIO_PinAFConfig(sensor->sda.port, sensor->sda.pinsource, sensor->sda.af);
    GPIO_PinAFConfig(sensor->scl.port, sensor->scl.pinsource, sensor->scl.af);

    // Set up GPIO for SDA and SCL, aka open-drain with pull-ups
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Pin = sensor->scl.pin;
    GPIO_Init(sensor->scl.port, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = sensor->sda.pin;
    GPIO_Init(sensor->sda.port, &GPIO_InitStructure);

    // Set up I2C parameters; 400khz, 7-bit addresses, "I2C mode" are important
    I2C_InitStructure.I2C_ClockSpeed = 400000;
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;

    // Initialize the I2C peripheral
	I2C_Cmd(sensor->peripheral, ENABLE);
    I2C_Init(sensor->peripheral, &I2C_InitStructure);
}

// Reconfigure the INA219s to run in high-speed mode
void INA219_SetHighSpeed(INA219* sensor, uint32_t clockspeed){
    // Save some typing
    I2C_TypeDef* periph = sensor->peripheral;
    I2C_InitTypeDef I2C_InitStructure;

    // Generate a START condition
    I2C_GenerateSTART(sensor->peripheral, ENABLE);

    // Ensure that we've taken control of the bus (EV5)
    if(INA219_EventTimeout(periph, I2C_EVENT_MASTER_MODE_SELECT, INA219_TIMEOUT,
                           ERROR))
        return;

    // Send the slave address with the R/nW flag high (read)
    I2C_Send7bitAddress(periph, 0x08, I2C_Direction_Receiver);

    // Defensively busy wait a bit
    for(volatile uint32_t a = SystemCoreClock/10000; a > 0; a--);

    // Set up I2C parameters; user-supplied clock , 7-bit addresses, "I2C mode"
    I2C_InitStructure.I2C_ClockSpeed = clockspeed;
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;

    // Initialize the I2C peripheral
    I2C_DeInit(periph);
	I2C_Cmd(periph, ENABLE);
    I2C_Init(periph, &I2C_InitStructure);
}

// Send a 16-bit word to the INA219
uint8_t INA219_WriteWord(INA219* sensor, uint8_t reg, uint16_t data)
{
    // Save some typing
    I2C_TypeDef* periph = sensor->peripheral;
    uint32_t address = sensor->address << 1;
    uint32_t timeout = INA219_TIMEOUT;

    // Make sure the bus isn't busy so that we don't screw up a transaction
    if(INA219_FlagTimeout(periph, I2C_FLAG_BUSY, timeout, SET))
        return INA219_I2C_BUSY;

    // Generate a START condition
    I2C_GenerateSTART(sensor->peripheral, ENABLE);

    // Ensure that we've taken control of the bus (EV5)
    if(INA219_EventTimeout(periph, I2C_EVENT_MASTER_MODE_SELECT, timeout,
                           ERROR))
        return INA219_I2C_TIMEOUTERROR;

    // Send the address with the R/nW flag
    I2C_Send7bitAddress(sensor->peripheral, address, I2C_Direction_Transmitter);

    // Make sure the INA219 ACKs our address (EV6)
    if(INA219_EventTimeout(periph, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED,
                          timeout, ERROR))
        return INA219_I2C_TIMEOUTERROR;

    // Write the desired register pointer byte
    I2C_SendData(periph, reg);

    // Wait for the byte to go out the door (EV8)
    if(INA219_EventTimeout(periph, I2C_EVENT_MASTER_BYTE_TRANSMITTING, timeout,
                           ERROR))
        return INA219_I2C_TIMEOUTERROR;

    // Write the most-significant byte of the word
    I2C_SendData(periph, (data & 0xFF00) >> 8);

    // Wait for the byte to go out the door (EV8)
    if(INA219_EventTimeout(periph, I2C_EVENT_MASTER_BYTE_TRANSMITTING, timeout,
                           ERROR))
        return INA219_I2C_TIMEOUTERROR;

    // Write the least-signficant byte of the word
    I2C_SendData(periph, (data & 0x00FF));

    // Wait for the byte to go out the door (EV8)
    if(INA219_EventTimeout(periph, I2C_EVENT_MASTER_BYTE_TRANSMITTING, timeout,
                           ERROR))
        return INA219_I2C_TIMEOUTERROR;

    // Generate a STOP condition
    I2C_GenerateSTOP(periph, ENABLE);

    // We made it!
    return INA219_SUCCESS;
}

// Set the address pointer and read a word from the INA219
uint8_t INA219_ReadWord(INA219* sensor, uint8_t reg, uint16_t* data, bool reqSp)
{
    // Save some typing
    I2C_TypeDef* periph = sensor->peripheral;
    uint32_t address = sensor->address << 1;
    uint32_t timeout = INA219_TIMEOUT;

    // Set the desired address
    if(reqSp){
        if(INA219_SetAddressPointer(sensor, reg) != INA219_SUCCESS)
            return INA219_FAILURE;
    }

    // Make sure the bus isn't busy so that we don't screw up a transaction
    if(INA219_FlagTimeout(periph, I2C_FLAG_BUSY, timeout, SET))
        return INA219_I2C_BUSY;

    // Generate a START condition
    I2C_GenerateSTART(periph, ENABLE);

    // Ensure that we've taken control of the bus (EV5)
    if(INA219_EventTimeout(periph, I2C_EVENT_MASTER_MODE_SELECT, timeout,
                           ERROR))
        return INA219_I2C_TIMEOUTERROR;

    // Send the slave address with the R/nW flag high (read)
    I2C_Send7bitAddress(periph, address, I2C_Direction_Receiver);

    // Enable acknowledge for the second byte
    I2C_AcknowledgeConfig(periph, ENABLE);

    // Wait for the address flag to be set
    if(INA219_FlagTimeout(periph, I2C_FLAG_ADDR, timeout, RESET))
        return INA219_I2C_TIMEOUTERROR;

    // Clear the ADDR flag in order to continue with reception
    periph->SR2;

    // Wait to receive a byte
    if(INA219_FlagTimeout(periph, I2C_FLAG_RXNE, timeout, RESET))
        return INA219_I2C_TIMEOUTERROR;

    // Read the most-significant byte of the word
    uint16_t msb = I2C_ReceiveData(periph);

    // Disable acknowledge for the second byte
    I2C_AcknowledgeConfig(periph, DISABLE);

    // Wait to receive a second byte
    if(INA219_FlagTimeout(periph, I2C_FLAG_RXNE, timeout, RESET))
        return INA219_I2C_TIMEOUTERROR;

    // Read the least-significant byte of the word
    uint16_t lsb = I2C_ReceiveData(periph);

    // Generate STOP condition
    I2C_GenerateSTOP(periph, ENABLE);

    // Write the results to the given address
    *data = ((msb << 8) & 0xFF00) | (lsb & 0x00FF);

    // We made it!
    return INA219_SUCCESS;
}

// The INA219 read command provides no facility for setting the regpointer.
// We must instead do a half-write to set the desired register. This is janky.
uint8_t INA219_SetAddressPointer(INA219* sensor, uint8_t reg)
{
    // Save some typing
    I2C_TypeDef* periph = sensor->peripheral;
    uint32_t address = sensor->address << 1;
    uint32_t timeout = INA219_TIMEOUT;

    // Make sure the bus isn't busy so that we don't screw up a transaction
    if(INA219_FlagTimeout(periph, I2C_FLAG_BUSY, timeout, SET))
        return INA219_I2C_BUSY;

    // Generate a START condition
    I2C_GenerateSTART(sensor->peripheral, ENABLE);

    // Ensure that we've taken control of the bus
    if(INA219_EventTimeout(periph, I2C_EVENT_MASTER_MODE_SELECT, timeout,
                           ERROR))
        return INA219_I2C_TIMEOUTERROR;

    // Send the address with R/nW set low (write)
    I2C_Send7bitAddress(sensor->peripheral, address, I2C_Direction_Transmitter);

    // Make sure the INA219 ACKs our address
    if(INA219_EventTimeout(periph, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED,
                          timeout, ERROR))
        return INA219_I2C_TIMEOUTERROR;

    // Write the desired register pointer byte
    I2C_SendData(periph, reg);

    // Wait for the byte to go out the door
    if(INA219_EventTimeout(periph, I2C_EVENT_MASTER_BYTE_TRANSMITTED, timeout,
                           ERROR))
        return INA219_I2C_TIMEOUTERROR;

    // Generate a STOP condition
    I2C_GenerateSTOP(periph, ENABLE);

    // We made it!
    return INA219_SUCCESS;
}
