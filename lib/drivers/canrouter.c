#include <canrouter.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define BOUND(input, lower, upper)	(MIN((upper), MAX((input), (lower))))

// Configure a new CAN filter based on provided mask and id
void CAN_InitHwF(Can* can, uint32_t mask, uint32_t id,
        bool ext, uint32_t fNum){
    // Assume users are lazy: clean up the given id with the mask
    id &= mask;
    // Standard versus extended identifier handling
    if(ext){
        id <<= 3; // Shift the extId all the way left (32-29)
        id &= 0xFFFFFFF8; // Make sure the bottom bits are clear
        id |= 0x4; // Set the extended identifier flag
        mask <<= 3;
        mask &= 0xFFFFFFF8; // Make sure the bottom bits are clear
    } else {
        id <<= 21; // Shift the stdId all the way left (32-11)
        id &= 0xFFE00000; // Make sure the bottom bits are clear
        mask <<= 21;
        mask &= 0xFFE00000; // Make sure the bottom bits are clear
    }
    mask |= 0x4; // Always distinguish between std and ext ID

    // Prepare a filter initialization struct to pass to the library function
    // Note the hack where we add 7 to fNum if we're talking about CAN2.
    // That's because the filter banks are shared between master and slave
    // peripherals and I've elected to evenly split them.
    CAN_FilterInitTypeDef filter = {
        .CAN_FilterIdHigh = ((uint16_t)((id >> 16) & 0xFFFF)),
        .CAN_FilterIdLow = ((uint16_t)(id & 0xFFFF)),
        .CAN_FilterMaskIdHigh = ((uint16_t)((mask >> 16) & 0xFFFF)),
        .CAN_FilterMaskIdLow = ((uint16_t)(mask & 0xFFFF)),
        .CAN_FilterFIFOAssignment = CAN_Filter_FIFO0,
        .CAN_FilterNumber = fNum,
        .CAN_FilterMode = CAN_FilterMode_IdMask,
        .CAN_FilterScale = CAN_FilterScale_32bit,
        .CAN_FilterActivation = ENABLE
    };
    // Initialize the filter with the above configuration
    CAN_FilterInit(&filter);
}

// Create a new dynamically allocated subscriber
static CanSubscriber* CAN_NewSubscriber(uint32_t mask, uint32_t id, bool ext,
        xQueueHandle queue){
    CanSubscriber* newSubscriber = malloc(sizeof(CanSubscriber));
    newSubscriber->mask = mask;
    newSubscriber->id = id;
    newSubscriber->ext = ext;
    newSubscriber->destQueue = queue;
    newSubscriber->next = 0;
    return newSubscriber;
}

// Configure and enable the appropriate CAN receive interrupts
void CAN_ConfigRxInterrupt(Can* can){
    // Select the appropriate interrupt offsets
    uint8_t rx0 = 0;
    uint8_t rx1 = 0;
    #ifdef STM32F30X
    rx0 = USB_LP_CAN1_RX0_IRQn;
    rx1 = CAN1_RX1_IRQn;
    #else
    if(can->canPeriph == CAN1){
        rx0 = CAN1_RX0_IRQn;
        rx1 = CAN1_RX1_IRQn;
    } else {
        rx0 = CAN2_RX0_IRQn;
        rx1 = CAN2_RX1_IRQn;
    }
    #endif
    // Configure the interrupt priority
    NVIC_InitTypeDef  NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = rx0;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0xf;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    NVIC_InitStructure.NVIC_IRQChannel = rx1;
    NVIC_Init(&NVIC_InitStructure);
    // Turn on the receive interrupt at the CAN peripheral
    CAN_ITConfig(can->canPeriph, CAN_IT_FMP0, ENABLE);
    CAN_ITConfig(can->canPeriph, CAN_IT_FMP1, ENABLE);
}

// Configure and enable the appropriate CAN transmit interrupts
void CAN_ConfigTxInterrupt(Can* can){
    // Select the appropriate interrupt offsets
    uint8_t tx = 0;
    #ifdef STM32F30X
    tx = USB_HP_CAN1_TX_IRQn;
    #else
    if(can->canPeriph == CAN1){
        tx = CAN1_TX_IRQn;
    } else {
        tx = CAN2_TX_IRQn;
    }
    #endif
    // Configure the interrupt priority
    NVIC_InitTypeDef  NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = tx;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0xf;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    // Enable the transmit mailbe empty (TME) interrupt
    CAN_ITConfig(can->canPeriph, CAN_IT_TME, ENABLE);
}

// Populate a CAN configuration structure with calculated baud rate settings
int32_t CAN_ComputeBaud(CAN_InitTypeDef* target, int32_t baud){
    // Gather some information about our clock speed. We're interested in PCLK.
    RCC_ClocksTypeDef RCC_ClocksStatus;
    RCC_GetClocksFreq(&RCC_ClocksStatus);
    uint32_t fPclk1 = RCC_ClocksStatus.PCLK1_Frequency;
    // The maximum number of time quanta (Ntq) per bit time is 25. Min is 3.
    // We like having flexibility to set the sample point, so we start our
    // checks at larger numbers of Tq per bit time.
    int32_t bestBrp = 0;
    int32_t bestNtq = 0;
    int32_t bestError = fPclk1;
    int32_t bestBaud = 0;
    for(int32_t Ntq = 25 ; Ntq >= 3; Ntq--){
        int32_t brp = BOUND((fPclk1 / (Ntq * baud)), 1, 1024);
        int32_t result = fPclk1 / (brp * Ntq);
        int32_t error = abs(result - baud);
        if(error < bestError){
            bestBrp = brp;
            bestNtq = Ntq;
            bestError = error;
            bestBaud = result;
        }
        if(error == 0){
            break;
        }
    }
    // Propagate what we've learned to the initialization struct
    uint32_t tbs1 = BOUND((uint32_t)((bestNtq - 1) * 0.75), 1, 16);
    uint32_t tbs2 = bestNtq - 1 - tbs1;
    target->CAN_SJW = CAN_SJW_4tq;
    target->CAN_BS1 = tbs1 - 1;
    target->CAN_BS2 = tbs2 - 1;
    target->CAN_Prescaler = bestBrp; // Do not subtract one; ST does it for us
    return bestBaud;
}

// Configure the CAN peripheral and its pins
void CAN_Config(Can* can, uint32_t baud, uint32_t queueSize){
    can->txCallback = NULL;
    can->txCallbackArg = NULL;
    // Connect CANTX and CANRX to the chosen CAN peripheral
    #ifdef STM32F30X
    GPIOSpeed_TypeDef GpioMediumSpeed = GPIO_Speed_Level_3;
    #else
    GPIOSpeed_TypeDef GpioMediumSpeed = GPIO_Speed_2MHz;
    #endif
    Pin_ConfigGpioPin(&(can->canTx), GPIO_Mode_AF, GpioMediumSpeed,
            GPIO_OType_PP, GPIO_PuPd_UP, true);
    Pin_ConfigGpioPin(&(can->canRx), GPIO_Mode_AF, GpioMediumSpeed,
            GPIO_OType_PP, GPIO_PuPd_UP, true);
    // Structures to aid in configuration
    CAN_InitTypeDef CAN_InitStructure;
    // Put the CAN peripheral in configuration mode
    CAN_DeInit(can->canPeriph);
    CAN_StructInit(&CAN_InitStructure);
    // Enable the CAN peripheral clock
    (*(can->rccfunc))(can->clock, ENABLE);
    // CAN peripheral configuration
    CAN_InitStructure.CAN_TTCM = DISABLE; // 0: No time triggered comms
    CAN_InitStructure.CAN_ABOM = ENABLE; // 1: Automatic exit from bus-off
    CAN_InitStructure.CAN_AWUM = ENABLE; // 1: Auto wakeup mode enabled
    CAN_InitStructure.CAN_NART = DISABLE; // 0: Auto retry on no ACK
    CAN_InitStructure.CAN_RFLM = ENABLE; // 1: Discard new when FIFO full
    CAN_InitStructure.CAN_TXFP = ENABLE; // 1: Transmit packets chronologically
    CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
    CAN_ComputeBaud(&CAN_InitStructure, baud);
    // Fire up the CAN peripheral, matching no packets by default
    // Note that interrupts are not yet enabled. Do that in a thread.
    CAN_Init(can->canPeriph, &CAN_InitStructure);
    // Initialize our parts of the can peripheral struct
    can->nextSubscriber = 0;
    can->txQueue = xQueueCreate(queueSize, sizeof(CanTxMsg));
}

// Add a packet to the outgoing queue
bool CAN_Enqueue(Can* can, CanTxMsg* msg, bool shouldCallback){
    // Call the tx callback if it exists
    if(shouldCallback && (can->txCallback != NULL))
        if((can->txCallback)(can, msg, can->txCallbackArg) == false)
            return true;

    // Try to send the message
    if(CAN_Transmit(can->canPeriph, msg) == CAN_TxStatus_NoMailBox){
        // No mailboxes were immediately available, so queue up the message
        return (bool)xQueueSendToBack(can->txQueue, msg, 1);
    } else {
        return true;
    }
}

// Subscribe to a type of message ID
void CAN_Subscribe(Can* can, uint32_t mask,
        uint32_t id, bool ext, xQueueHandle queue){
    // Add to the end of the list of subscribers
    if(can->nextSubscriber == 0){
        can->nextSubscriber = CAN_NewSubscriber(mask, id, ext, queue);
    } else {
        CanSubscriber* curr = can->nextSubscriber;
        CanSubscriber* prev = curr;
        while(curr != 0){
            prev = curr;
            curr = curr->next;
        }
        prev->next = CAN_NewSubscriber(mask, id, ext, queue);
    }
}

// Send a message to all subscribers. Useful for injecting local packets.
portBASE_TYPE CAN_NotifySubscribers(Can* can, CanRxMsg* rxMsg, uint8_t rxFifo){
    portBASE_TYPE retval;
    // Bail if we have no subscribers
    if(can->nextSubscriber == 0){
        return pdFALSE;
    }
    // Traverse the linked list of subscribers
    CanSubscriber* curr = can->nextSubscriber;
    while(curr != 0){
        // Run software filtering on the incoming data and enqueue accordingly
        bool ext = (rxMsg->IDE == CAN_Id_Standard) ? false : true;
        uint32_t id = ext ? rxMsg->ExtId : rxMsg->StdId;
        if((ext == curr->ext) && ((id & curr->mask) == curr->id)){
            xQueueSendToBackFromISR(curr->destQueue, rxMsg, &retval);
        }
        curr = curr->next;
    }
    return retval;
}

// Call from the transmit interrupt
portBASE_TYPE CAN_TxIsr(Can* can){
    // Clear the mailbox empty bit
    CAN_ClearITPendingBit(can->canPeriph, CAN_IT_TME);
    // Check to see if we've got more packets to send
    static portBASE_TYPE numItems;
    static portBASE_TYPE xHigherPriorityTaskWoken;
    numItems = uxQueueMessagesWaitingFromISR(can->txQueue);
    if(numItems > 0){
        // There's a packet to send and an empty mailbox to stuff... ship it!
        CanTxMsg msg;
        xQueueReceiveFromISR(can->txQueue, &msg, &xHigherPriorityTaskWoken);
        CAN_Transmit(can->canPeriph, &msg);
    }
    return xHigherPriorityTaskWoken;
}

// Call from the receive interrupt
portBASE_TYPE CAN_RxIsr(Can* can, uint8_t rxFifo){
    static CanRxMsg rxMsg;
    CAN_Receive(can->canPeriph, rxFifo, &rxMsg);
    return CAN_NotifySubscribers(can, &rxMsg, rxFifo);
}
