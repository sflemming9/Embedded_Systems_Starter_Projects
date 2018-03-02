#pragma once

// Outgoing can messages
extern xQueueHandle CanTxQueue;
// All incoming messages
extern xQueueHandle CanRxQueue;

void CAN_Setup();