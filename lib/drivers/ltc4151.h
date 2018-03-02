/**
 * Function prototypes for the LTC4151 High Voltage I2C Current and
 * Voltage Monitor.
 */
 
#ifndef __LTC4151_H
#define __LTC4151_H

#include "stm32f4xx_i2c.h"

// Register addresses for the different values
#define LTC4151_REG_SENSE_MSB           0x00
#define LTC4151_REG_SENSE_LSB           0x01
#define LTC4151_REG_V_IN_MSB            0x02
#define LTC4151_REG_V_IN_LSB            0x03
#define LTC4151_REG_ADIN_MSB            0x04
#define LTC4151_REG_ADIN_LSB            0x05
#define LTC4151_REG_CONTROL             0x06
     
// Scale divisions for the voltage readings
#define LTC4151_SCALE_SENSE             20 // microvolts
#define LTC4151_SCALE_V_IN              25 // milivolts
#define LTC4151_SCALE_ADIN              500 // microvolts

#define LTC4151_ADDR                    0xCE
#define LTC4151_I2C_SPEED               100000

#define LTC4151_I2C                     I2C1
#define LTC4151_I2C_CLK	                RCC_APB1Periph_I2C1
#define LTC4151_I2C_SCL_PIN             GPIO_Pin_8
#define LTC4151_I2C_SCL_GPIO_PORT       GPIOB
#define LTC4151_I2C_SCL_GPIO_CLK        RCC_AHB1Periph_GPIOB
#define LTC4151_I2C_SCL_SOURCE          GPIO_PinSource8
#define LTC4151_I2C_SCL_AF              GPIO_AF_I2C1
#define LTC4151_I2C_SDA_PIN             GPIO_Pin_7
#define LTC4151_I2C_SDA_GPIO_PORT       GPIOB
#define LTC4151_I2C_SDA_GPIO_CLK        RCC_AHB1Periph_GPIOB
#define LTC4151_I2C_SDA_SOURCE          GPIO_PinSource7
#define LTC4151_I2C_SDA_AF              GPIO_AF_I2C1

// CONTROL register bits
#define LTC4151_PAGE_RW                 0x08
#define LTC4151_TEST_MODE               0x10
#define LTC4151_VIN_SNAPSHOT            0x20
#define LTC4151_ADIN_SNAPSHOT           0x40
#define LTC4151_SNAPSHOT_MODE           0x80

void LTC4151_DeInit(void);
void LTC4151_Init(void);
void LTC4151_LowLevel_Init(void);

// Reads 2 bytes via I2C and converts them into a 12-bit integer
int LTC4151_ReadWord(uint16_t deviceAddr, uint8_t reg_addr, void *buf);

// Returns the 12-bit raw voltage reading for the given register
uint16_t LTC4151_ReadVoltage(uint16_t deviceAddr, uint8_t reg_addr);

// Returns SENSE voltage in number of microV
uint32_t LTC4151_Sense_Voltage(uint16_t deviceAddr);

// Returns V_in voltage in number of mV
uint32_t LTC4151_V_In_Voltage(uint16_t deviceAddr);

// Returns ADin voltage in number of microV
uint32_t LTC4151_ADin_Voltage(uint16_t deviceAddr);

// Return the SENSE current in volts given a resistance value
float LTC4151_Sense_Current(uint16_t deviceAddr, float resist);

#endif