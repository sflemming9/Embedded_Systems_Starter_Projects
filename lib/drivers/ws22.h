#pragma once

#include <FreeRTOS.h>
#include <queue.h>
#include <stdbool.h>
#include <canrouter.h>

#ifdef STM32F4XX
	#include <stm32f4xx.h>
#elif STM32F30X
	#include <stm32f30x.h>
#elif STM32F2XX
    #include <stm32f2xx.h>
#endif

// Struct to unpack Tritium's standard CAN ID format
typedef __packed struct WS22_CanId_Bf {
    unsigned int msgId:5;
    unsigned int devId:6;
    unsigned int padding:21;
} WS22_CanIdBf;

typedef union WS22_CanId_T {
    uint32_t asInt;
    WS22_CanIdBf asFields;
} WS22_CanId;

// Struct to unpack error flags
typedef __packed struct WS22_ErrFlags_T {
    unsigned int hardwareOverCurrent:1;
    unsigned int softwareOverCurrent:1;
    unsigned int dcBusOverVoltage:1;
    unsigned int badMotorPositionSequence:1;
    unsigned int watchdogReset:1;
    unsigned int configReadError:1;
    unsigned int supplyUnderVoltage:1;
    unsigned int desaturationFault:1;
    unsigned int padding:8;
} WS22_ErrFlags;

// Struct to unpack limit flags
typedef __packed struct WS22_LimitFlags_T {
    unsigned int outputVoltagePwm:1;
    unsigned int motorCurrent:1;
    unsigned int velocity:1;
    unsigned int busCurrent:1;
    unsigned int busVoltageUpperLimit:1;
    unsigned int busVoltageLowerLimit:1;
    unsigned int motorTemperature:1;
    unsigned int padding:9;
} WS22_LimitFlags;

// Struct to fully represent Wavesculptor22 state
typedef struct WS22_T {
    Can* can;
    xQueueHandle rxQueue;
    uint32_t mcBase;
    uint32_t dcBase;
    int64_t cycleCountLastMessage;
    float cmdMotorCurrent;
    float cmdMotorVelocity;
    float cmdBusCurrent;
    uint32_t serialNumber;
    char tritiumId[4];
    uint8_t rxErrCount;
    uint8_t txErrCount;
    uint16_t activeMotor;
    WS22_ErrFlags errorFlags;
    WS22_LimitFlags limitFlags;
    float busCurrent;
    float busVoltage;
    float vehicleVelocity;
    float motorVelocity;
    float phaseCurrentB;
    float phaseCurrentC;
    float voltageD;
    float voltageQ;
    float currentD;
    float currentQ;
    float emfD;
    float emfQ;
    float rail15v0;
    float rail3v3;
    float rail1v9;
    float ipmTemp;
    float motorTemp;
    float cpuTemp;
    float dcAmpHours;
    float odometer;
    float slipSpeed;
    void (*updateCallback)(struct WS22_T* ws22, WS22_CanId msgId);
} WS22;

// Initialize the WS22 struct
// Warning: Canrouter must already be initialized, because we're going to add
// a subscriber.
void WS22_Init(WS22* ws22, Can* can, uint32_t mcBase, uint32_t dcBase);

// Register a function that gets called every time we update state
void WS22_RegisterCallback(WS22* ws22,
    void (*callback)(struct WS22_T* ws22, WS22_CanId msgId));

// Send motor drive commands to controller, update the local copies
bool WS22_SendMotorDrive(WS22* ws22, float motorCurrent, float motorVelocity);

// Send motor power commands to the controller, update the local copy
bool WS22_SendMotorPower(WS22* ws22, float busCurrent);

// Reset the controller
bool WS22_SendReset(WS22* ws22);