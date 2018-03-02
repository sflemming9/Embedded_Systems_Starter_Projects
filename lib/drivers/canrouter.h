#pragma once

#include <stdbool.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <pin.h>

#ifdef STM32F4XX
	#include <stm32f4xx.h>
#elif STM32F30X
	#include <stm32f30x.h>
#elif STM32F2XX
    #include <stm32f2xx.h>
#endif

typedef struct CanSubscriberT {
    struct CanSubscriberT* next;
    uint32_t mask;
    uint32_t id;
    bool ext;
    xQueueHandle destQueue;
} CanSubscriber;

typedef struct CanT {
    CAN_TypeDef* canPeriph;
    uint32_t clock;
    void (*rccfunc)(uint32_t peripheral, FunctionalState NewState);
    Pin canTx;
    Pin canRx;
    bool (*txCallback)(struct CanT* can, CanTxMsg* txMsg, void* arg);
    void* txCallbackArg;
    bool initialized;
    struct CanSubscriberT* nextSubscriber;
    xQueueHandle txQueue;
} Can;

// Configure a new CAN filter based on provided mask and id
void CAN_InitHwF(Can* can, uint32_t mask, uint32_t id,
        bool ext, uint32_t fNum);

// Configure the CAN peripheral and its pins
void CAN_Config(Can* can, uint32_t baud, uint32_t queueSize);

// Add a packet to the outgoing queue
bool CAN_Enqueue(Can* can, CanTxMsg* msg, bool shouldCallback);

// Subscribe to a type of message ID
// match = ((packetId & mask)==(id))
// Matching packets will be put in to the returned queue.
void CAN_Subscribe(Can* can, uint32_t mask, uint32_t id, bool ext,
        xQueueHandle queue);

// Configure and enable the appropriate CAN transmit interrupts
void CAN_ConfigTxInterrupt(Can* can);

// Configure and enable the appropriate CAN receive interrupts
void CAN_ConfigRxInterrupt(Can* can);

// Send a message to all subscribers. Useful for injecting local packets.
portBASE_TYPE CAN_NotifySubscribers(Can* can, CanRxMsg* rxMsg, uint8_t rxFifo);

// Call from the transmit interrupt
portBASE_TYPE CAN_TxIsr(Can* can);

// Call from the receive interrupt
portBASE_TYPE CAN_RxIsr(Can* can, uint8_t rxFifo);
