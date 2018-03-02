#include "ltc4151.h"
#include <global.h>

#define TIMEOUT_MAX         0x3000
#define LTC4151_TIMEOUT     -1
#define LTC4151_SUCCESS     0

uint32_t LTC4151_TimeOut = TIMEOUT_MAX;

void LTC4151_DeInit(void)
{
    I2C_Cmd(LTC4151_I2C, DISABLE);
    I2C_DeInit(LTC4151_I2C);
    RCC_APB1PeriphClockCmd(LTC4151_I2C_CLK, DISABLE);
}

void LTC4151_Init(void)
{	
    LTC4151_LowLevel_Init();
	
    I2C_DeInit(LTC4151_I2C);

    I2C_InitTypeDef I2C_InitStructure;

    /* LTC4151_I2C Init */
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = LTC4151_I2C_SPEED;

    I2C_Cmd(LTC4151_I2C, ENABLE);
    I2C_Init(LTC4151_I2C, &I2C_InitStructure);
}

void LTC4151_LowLevel_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB1PeriphClockCmd(LTC4151_I2C_CLK, ENABLE);
    RCC_AHB1PeriphClockCmd(LTC4151_I2C_SCL_GPIO_CLK | LTC4151_I2C_SDA_GPIO_CLK, ENABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
    RCC_APB1PeriphResetCmd(LTC4151_I2C_CLK, ENABLE);
    RCC_APB1PeriphResetCmd(LTC4151_I2C_CLK, DISABLE);

    /* GPIO configuration */
    GPIO_PinAFConfig(LTC4151_I2C_SCL_GPIO_PORT, LTC4151_I2C_SCL_SOURCE, LTC4151_I2C_SCL_AF);
    GPIO_PinAFConfig(LTC4151_I2C_SDA_GPIO_PORT, LTC4151_I2C_SDA_SOURCE, LTC4151_I2C_SDA_AF);  

    /* Configure SCL */
    GPIO_InitStructure.GPIO_Pin = LTC4151_I2C_SCL_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(LTC4151_I2C_SCL_GPIO_PORT, &GPIO_InitStructure);
          
    /* Configure SDA */
    GPIO_InitStructure.GPIO_Pin = LTC4151_I2C_SDA_PIN;
    GPIO_Init(LTC4151_I2C_SDA_GPIO_PORT, &GPIO_InitStructure);
}

int LTC4151_ReadWord(uint16_t deviceAddr, uint8_t reg_addr, void *buf)
{
    uint16_t msb = 0x0;
    uint16_t lsb = 0x0;
    
    // Ensure that bus is not busy
    LTC4151_TimeOut = TIMEOUT_MAX;
    while (I2C_GetFlagStatus(LTC4151_I2C, I2C_FLAG_BUSY))
    {
        if (LTC4151_TimeOut-- == 0) return LTC4151_TIMEOUT;
    }
    
    // Send START
    I2C_GenerateSTART(LTC4151_I2C, ENABLE);

    // Master mode successfully selected
    LTC4151_TimeOut = TIMEOUT_MAX;
    while (!I2C_CheckEvent(LTC4151_I2C, I2C_EVENT_MASTER_MODE_SELECT))
    {
        if (LTC4151_TimeOut-- == 0) return LTC4151_TIMEOUT;
    }

    // Send slave address
    I2C_Send7bitAddress(LTC4151_I2C, deviceAddr, I2C_Direction_Transmitter);
    
    // Waits for transmitter mode to be selected
    LTC4151_TimeOut = TIMEOUT_MAX;
    while (!I2C_CheckEvent(LTC4151_I2C, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    {
        if (LTC4151_TimeOut-- == 0) return LTC4151_TIMEOUT;
    }

    // Send register address of MSB
    I2C_SendData(LTC4151_I2C, reg_addr);

    LTC4151_TimeOut = TIMEOUT_MAX;
    while (!I2C_CheckEvent(LTC4151_I2C, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    {
        if (LTC4151_TimeOut-- == 0) return LTC4151_TIMEOUT;
    }

    // Send START
    I2C_GenerateSTART(LTC4151_I2C, ENABLE);

    // Master mode successfully selected
    LTC4151_TimeOut = TIMEOUT_MAX;
    while (!I2C_CheckEvent(LTC4151_I2C, I2C_EVENT_MASTER_MODE_SELECT))
    {
        if (LTC4151_TimeOut-- == 0) return LTC4151_TIMEOUT;
    }

    // Send address again to enter receive mode
    I2C_Send7bitAddress(LTC4151_I2C, deviceAddr, I2C_Direction_Receiver);
    
    // Address flag set
    LTC4151_TimeOut = TIMEOUT_MAX;
    while (I2C_GetFlagStatus(LTC4151_I2C, I2C_FLAG_ADDR) == RESET)
    {
        if (LTC4151_TimeOut-- == 0) return LTC4151_TIMEOUT;
    }

    // Enable ACK
    I2C_AcknowledgeConfig(LTC4151_I2C, ENABLE);
    
    // Clear address flag
    (void) LTC4151_I2C->SR2;
    
    // Data received
    LTC4151_TimeOut = TIMEOUT_MAX;
    while (I2C_GetFlagStatus(LTC4151_I2C, I2C_FLAG_RXNE) == RESET)
    {
        if (LTC4151_TimeOut-- == 0) return LTC4151_TIMEOUT;
    }

    // Read first byte
    msb = I2C_ReceiveData(LTC4151_I2C);
    msb = msb << 4;

    // Enable ACK
    I2C_AcknowledgeConfig(LTC4151_I2C, DISABLE);
    
    // Data received
    LTC4151_TimeOut = TIMEOUT_MAX;
    while (I2C_GetFlagStatus(LTC4151_I2C, I2C_FLAG_RXNE) == RESET)
    {
        if (LTC4151_TimeOut-- == 0) return LTC4151_TIMEOUT;
    }
    
    // Read second byte
    lsb = I2C_ReceiveData(LTC4151_I2C);
    lsb = lsb & 0x0f;

    // End communication
    I2C_GenerateSTOP(LTC4151_I2C, ENABLE);

    *(uint16_t *) buf = msb | lsb;    
    
    return LTC4151_SUCCESS;
}

uint16_t LTC4151_ReadVoltage(uint16_t deviceAddr, uint8_t reg_addr)
{
    uint16_t value = 0x0;
    if (LTC4151_ReadWord(deviceAddr, reg_addr, &value) < 0)
        return 0;
    return value;
}

uint32_t LTC4151_Sense_Voltage(uint16_t deviceAddr)
{
    uint16_t raw_v = LTC4151_ReadVoltage(deviceAddr, LTC4151_REG_SENSE_MSB);
    uint32_t v = LTC4151_SCALE_SENSE * raw_v;
    return v;
}

uint32_t LTC4151_V_In_Voltage(uint16_t deviceAddr)
{
    uint16_t raw_v = LTC4151_ReadVoltage(deviceAddr, LTC4151_REG_V_IN_MSB);
    uint32_t v = LTC4151_SCALE_V_IN * raw_v;
    return v;
}

uint32_t LTC4151_ADin_Voltage(uint16_t deviceAddr)
{
    uint16_t raw_v = LTC4151_ReadVoltage(deviceAddr, LTC4151_REG_ADIN_MSB);
    uint32_t v = LTC4151_SCALE_ADIN * raw_v;
    return v;
}

float LTC4151_Sense_Current(uint16_t deviceAddr, float resist)
{
    uint32_t v = LTC4151_Sense_Voltage(deviceAddr);
    float vf = v / 1000000.0f;
    return vf / resist;
}
