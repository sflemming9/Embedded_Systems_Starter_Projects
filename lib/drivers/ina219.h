#pragma once

#include <stdbool.h>
#include <pin.h>

#ifdef STM32F4XX
	#include <stm32f4xx.h>
#elif STM32F30X
	#include <stm32f30x.h>
#elif STM32F2XX
    #include <stm32f2xx.h>
#endif

// Return values
#define INA219_SUCCESS 0
#define INA219_FAILURE 1
#define INA219_I2C_BUSY 2
#define INA219_I2C_TIMEOUTERROR 3

// INA219 registers
#define INA219_REG_CONF 0x00
#define INA219_REG_SHUNT 0x01
#define INA219_REG_BUS 0x02
#define INA219_REG_POWER 0x03
#define INA219_REG_CURRENT 0x04
#define INA219_REG_CALIBRATION 0x05

// INA219 configuration register bits
#define INA219_CONFBIT_RESET 0x8000

#define INA219_CONFBIT_BUSRANGE_16V 0x0000
#define INA219_CONFBIT_BUSRANGE_32V 0x2000

#define INA219_CONFBIT_PGA_40MV 0x0000
#define INA219_CONFBIT_PGA_80MV 0x0800
#define INA219_CONFBIT_PGA_160MV 0x1000
#define INA219_CONFBIT_PGA_320MV 0x1800

#define INA219_CONFBIT_SAMP_84US 0x0000 // 9-bit conversion
#define INA219_CONFBIT_SAMP_148US 0x0008 // 10-bit conversion
#define INA219_CONFBIT_SAMP_276US 0x0010 // 11-bit conversion
#define INA219_CONFBIT_SAMP_532US 0x0040 // 12-bit conversion
#define INA219_CONFBIT_SAMP_1060US 0x0048 // 2 averaged 12-bit conversions
#define INA219_CONFBIT_SAMP_2130US 0x0050 // 4 averaged 12-bit conversions
#define INA219_CONFBIT_SAMP_4260US 0x0058 // 8 averaged 12-bit conversions
#define INA219_CONFBIT_SAMP_8510US 0x0060 // 16 averaged 12-bit conversions
#define INA219_CONFBIT_SAMP_17020US 0x0068 // 32 averaged 12-bit conversions
#define INA219_CONFBIT_SAMP_34050US 0x0070 // 64 averaged 12-bit conversions
#define INA219_CONFBIT_SAMP_68100US 0x0078 // 128 averaged 12-bit conversions

#define INA219_CONFBIT_MODE_POWERDOWN 0x0000
#define INA219_CONFBIT_MODE_SHUNT_TRIGGERED 0x0001
#define INA219_CONFBIT_MODE_BUS_TRIGGERED 0x0002
#define INA219_CONFBIT_MODE_BOTH_TRIGGERED 0x0003
#define INA219_CONFBIT_MODE_ADCOFF 0x0004
#define INA219_CONFBIT_MODE_SHUNT_CONTINUOUS 0x0005
#define INA219_CONFBIT_MODE_BUS_CONTINUOUS 0x0006
#define INA219_CONFBIT_MODE_BOTH_CONTINUOUS 0x0007

// Externally defined SystemCoreClock
extern uint32_t SystemCoreClock;

// High-level representation of the sensor
typedef struct
{
    I2C_TypeDef* peripheral;
		void	(*rccfunc)(uint32_t peripheral, FunctionalState NewState);
    uint32_t clock;
    Pin sda;
    Pin scl;
    uint8_t address;
	uint16_t samplingTime;
    uint16_t mode;
	uint16_t busRange;
	uint16_t pgaGain;
	float calCurrentSlope;
	float calCurrentOffset;
    float current;
    float voltage;
    bool monitored;
    char* name;
} INA219;

// Deinitialize the sensor
void INA219_DeInit(INA219* sensor);

// Initialize the sensor
void INA219_Init(INA219* sensor);

// Reconfigure the INA219s to run in high-speed mode
void INA219_SetHighSpeed(INA219* sensor, uint32_t clockspeed);

// Send a 16-bit word to the INA219
uint8_t INA219_WriteWord(INA219* sensor, uint8_t reg, uint16_t data);

// Set the address pointer and read a word from the INA219
uint8_t INA219_ReadWord(INA219* sensor, uint8_t reg, uint16_t* data, bool reqSp);

// The INA219 read command provides no facility for setting the regpointer.
// We must instead do a half-write to set the desired register. This is janky.
uint8_t INA219_SetAddressPointer(INA219* sensor, uint8_t reg);
