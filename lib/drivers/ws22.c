#include "ws22.h"
#include <task.h>
#include <string.h>
#include <bsp.h>

// Parse incoming motor controller CAN messages
void WS22_ParseMcMsg(WS22* ws22, CanRxMsg* rxMsg){
    WS22_CanId canId;
    canId.asInt = rxMsg->StdId;

    switch(canId.asFields.msgId){
        case 0: // Identification information
            memcpy(&(ws22->serialNumber), &(rxMsg->Data[4]), 4);
            memcpy(&(ws22->tritiumId), &(rxMsg->Data[0]), 4);
            break;
        case 1: // Status information
            memcpy(&(ws22->rxErrCount), &(rxMsg->Data[7]), 1);
            memcpy(&(ws22->txErrCount), &(rxMsg->Data[6]), 1);
            memcpy(&(ws22->activeMotor), &(rxMsg->Data[4]), 2);
            memcpy(&(ws22->errorFlags), &(rxMsg->Data[2]), 2);
            memcpy(&(ws22->limitFlags), &(rxMsg->Data[0]), 2);
            break;
        case 2: // Bus measurement
            memcpy(&(ws22->busCurrent), &(rxMsg->Data[4]), 4);
            memcpy(&(ws22->busVoltage), &(rxMsg->Data[0]), 4);
            break;
        case 3: // Velocity measurement
            memcpy(&(ws22->vehicleVelocity), &(rxMsg->Data[4]), 4);
            memcpy(&(ws22->motorVelocity), &(rxMsg->Data[0]), 4);
            break;
        case 4: // Phase current measurement
            memcpy(&(ws22->phaseCurrentB), &(rxMsg->Data[4]), 4);
            memcpy(&(ws22->phaseCurrentC), &(rxMsg->Data[0]), 4);
            break;
        case 5: // Motor voltage vector measurement
            memcpy(&(ws22->voltageD), &(rxMsg->Data[4]), 4);
            memcpy(&(ws22->voltageQ), &(rxMsg->Data[0]), 4);
            break;
        case 6: // Motor current vector measurement
            memcpy(&(ws22->currentD), &(rxMsg->Data[4]), 4);
            memcpy(&(ws22->currentQ), &(rxMsg->Data[0]), 4);
            break;
        case 7: // Motor back EMF measurement / prediction
            memcpy(&(ws22->emfD), &(rxMsg->Data[4]), 4);
            memcpy(&(ws22->emfQ), &(rxMsg->Data[0]), 4);
            break;
        case 8: // 15v rail measurement
            memcpy(&(ws22->rail15v0), &(rxMsg->Data[4]), 4);
            break;
        case 9: // 3.3v and 1.9v rail measurement
            memcpy(&(ws22->rail3v3), &(rxMsg->Data[4]), 4);
            memcpy(&(ws22->rail1v9), &(rxMsg->Data[0]), 4);
            break;
        case 11: // Power electronics and motor temperature measurement
            memcpy(&(ws22->ipmTemp), &(rxMsg->Data[4]), 4);
            memcpy(&(ws22->motorTemp), &(rxMsg->Data[0]), 4);
            break;
        case 12: // Processor temperature measurement
            memcpy(&(ws22->cpuTemp), &(rxMsg->Data[0]), 4);
            break;
        case 14: // Odometer and coulomb counter measurement
            memcpy(&(ws22->dcAmpHours), &(rxMsg->Data[4]), 4);
            memcpy(&(ws22->odometer), &(rxMsg->Data[0]), 4);
            break;
        case 23: // Slip speed measurement
            memcpy(&(ws22->slipSpeed), &(rxMsg->Data[4]), 4);
            break;
        default:
            break;
    }
}

// Parse incoming driver controls CAN messages
void WS22_ParseDcMsg(WS22* ws22, CanRxMsg* rxMsg){
    WS22_CanId canId;
    canId.asInt = rxMsg->StdId;

    switch(canId.asFields.msgId){
        case 1: // Motor drive command
            memcpy(&(ws22->cmdMotorCurrent), &(rxMsg->Data[4]), 4);
            memcpy(&(ws22->cmdMotorVelocity), &(rxMsg->Data[0]), 4);
            break;
        case 2: // Motor power command
            memcpy(&(ws22->cmdBusCurrent), &(rxMsg->Data[4]), 4);
            break;
        default:
            break;
    }
}

// Task to receive and process WS22 packets
void WS22_ReceiveTask(void* pvParameters){
    WS22* ws22 = (WS22*)pvParameters;
    CanRxMsg rxMsg;
    WS22_CanId canId;

    while(1){
        // Wait for a message to come in
        if(xQueueReceive(ws22->rxQueue, &rxMsg, portMAX_DELAY) == pdPASS){
            // Parse the incoming message
            canId.asInt = rxMsg.StdId;
            if(canId.asFields.devId << 5 == ws22->mcBase)
                WS22_ParseMcMsg(ws22, &rxMsg);
            else if(canId.asFields.devId << 5 == ws22->dcBase)
                WS22_ParseDcMsg(ws22, &rxMsg);
            // Timestamp the transaction
            ws22->cycleCountLastMessage = BSP_TotalClockCycles();
            // Call the update callback if we have one
            if(ws22->updateCallback != NULL)
                (*(ws22->updateCallback))(ws22, canId);
        }
    }
}

// Initialize the WS22 struct
// Warning: Canrouter must already be initialized, because we're going to add
// a subscriber.
void WS22_Init(WS22* ws22, Can* can, uint32_t mcBase, uint32_t dcBase){
    memset(ws22, 0, sizeof(ws22));
    ws22->can = can;
    ws22->mcBase = mcBase;
    ws22->dcBase = dcBase;
    ws22->rxQueue = xQueueCreate(16, sizeof(CanRxMsg));
    xTaskCreate(WS22_ReceiveTask, (const signed char *)("WS22task"), 512, ws22, tskIDLE_PRIORITY + 1, NULL);
    CAN_Subscribe(ws22->can, 0x7E0, mcBase, false, ws22->rxQueue);
    CAN_Subscribe(ws22->can, 0x7E0, dcBase, false, ws22->rxQueue);
}

// Register a function that gets called every time we update state
void WS22_RegisterCallback(WS22* ws22,
    void (*callback)(struct WS22_T* ws22, WS22_CanId msgId)){
    ws22->updateCallback = callback;
}

// Send motor drive commands to controller, update the local copies
bool WS22_SendMotorDrive(WS22* ws22, float motorCurrent, float motorVelocity){
    CanTxMsg txMsg;
    txMsg.RTR = CAN_RTR_Data;
    txMsg.IDE = CAN_Id_Standard;
    txMsg.StdId = ws22->dcBase + 1;
    txMsg.DLC = 8;
    memcpy(&(txMsg.Data[4]), &motorCurrent, sizeof(float));
    memcpy(&(txMsg.Data[0]), &motorVelocity, sizeof(float));

    return CAN_Enqueue(ws22->can, &txMsg, true);
}

// Send motor power commands to the controller, update the local copy
bool WS22_SendMotorPower(WS22* ws22, float busCurrent){
    float dummy = 0.0f;
    CanTxMsg txMsg;
    txMsg.RTR = CAN_RTR_Data;
    txMsg.IDE = CAN_Id_Standard;
    txMsg.StdId = ws22->dcBase + 2;
    txMsg.DLC = 8;
    memcpy(&(txMsg.Data[4]), &busCurrent, sizeof(float));
    memcpy(&(txMsg.Data[0]), &dummy, sizeof(float));

    return CAN_Enqueue(ws22->can, &txMsg, true);
}

// Reset the controller
bool WS22_SendReset(WS22* ws22){
    CanTxMsg txMsg;
    char* msg = "RESETWS";
    txMsg.RTR = CAN_RTR_Data;
    txMsg.IDE = CAN_Id_Standard;
    txMsg.StdId = ws22->mcBase + 25;
    txMsg.DLC = 8;
    strncpy((void*)(&(txMsg.Data[0])), msg, 8);

    return CAN_Enqueue(ws22->can, &txMsg, true);
}