#include "global.h"     // Do I need this for the freertos stuff?
// @rachel: it might make more sense to specifically include the things you're
// actually using
#include "can.h"

#define CAN_TXLEN		128
#define CAN_RXLEN		128

xQueueHandle CanTxQueue;
xQueueHandle CanRxQueue;

// Set up the CAN peripheral
// PA11 (CANRX) as digital input with integrated pull-up
// PA12 (CANTX) as low speed digital push-pull output
// @rachel: This seems like bad form; the CAN peripheral can connect
// to multiple pins. 

void CAN_Setup()
{    
    // Create CAN message buffers
	// @rachel: Only one message deep?
    // @sasha: No, CAN_TXLEN messages deep
    CanTxQueue = xQueueCreate(CAN_TXLEN, sizeof(CanTxMsg));
    CanRxQueue = xQueueCreate(CAN_RXLEN, sizeof(CanTxMsg));
    
    CAN_InitTypeDef CAN_InitStructure;
    CAN_FilterInitTypeDef CAN_FilterInitStructure;

    // Data structure to represent GPIO configuration information
    GPIO_InitTypeDef  GPIO_InitStructure;

    // Put the CAN peripheral in configuration mode
    CAN_DeInit(CAN1);
    CAN_StructInit(&CAN_InitStructure);

    // Enable GPIO bank A and AFIO clocks
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    // Enable the CAN peripheral clock
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);

    // Enable the remap of CAN1 to PA11 and PA12
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_CAN1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource12, GPIO_AF_CAN1); 

    // Indicate that we want digital input with pull-up on PA11 (CANRX)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // Indicate that we want low speed digital push-pull output on PA12 (CANTX)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // CAN peripheral initialization (125kbps)
	// @rachel: This is pretty easy to compute on-the-fly. Why not pass it in?
    CAN_InitStructure.CAN_TTCM = DISABLE;
    CAN_InitStructure.CAN_ABOM = DISABLE;
    CAN_InitStructure.CAN_AWUM = DISABLE;
    CAN_InitStructure.CAN_NART = DISABLE;
    CAN_InitStructure.CAN_RFLM = DISABLE;
    CAN_InitStructure.CAN_TXFP = DISABLE;
    CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
    CAN_InitStructure.CAN_SJW = CAN_SJW_4tq;
    CAN_InitStructure.CAN_BS1 = CAN_BS1_14tq;
    CAN_InitStructure.CAN_BS2 = CAN_BS2_3tq;
    CAN_InitStructure.CAN_Prescaler = 16;
    
    // Enable the peripheral
	// @rachel: Always CAN1? Y u no use CAN2 sometimes? Sounds like another
	// parameter.
    CAN_Init(CAN1, &CAN_InitStructure);

    // CAN filter initialization
    CAN_FilterInitStructure.CAN_FilterNumber=0;
    CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask;
    CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit;
    CAN_FilterInitStructure.CAN_FilterIdHigh=0x0000;
    CAN_FilterInitStructure.CAN_FilterIdLow= 0x00;   
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdLow= 0x00;  
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_FIFO0;
    CAN_FilterInitStructure.CAN_FilterActivation=ENABLE;
    CAN_FilterInit(&CAN_FilterInitStructure);

    // Interrupt on CAN receive and CAN transmit
    CAN_ITConfig(CAN1, CAN_IT_FMP0 | CAN_IT_TME, ENABLE);
    
    // Configure CAN interrupt priority
    NVIC_InitTypeDef  NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x7;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    NVIC_InitStructure.NVIC_IRQChannel = CAN1_TX_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x7;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/* Check the status of CAN message mailboxes. If none are available, enqueue
   the message and turn on an interrupt so that we empty the queue as messages
   go out. If mailboxes are available, send immediately.*/
void CAN_Enqueue(CAN_TypeDef* CANx, CanTxMsg* TxMessage, xQueueHandle queue)
{
    if(CAN_Transmit(CANx, TxMessage) == CAN_NO_MB)
    {
		// Append to the queue, blocking if the queue is full.
		// Time out after 5 SysTicks (usually 1ms/SysTick).
        xQueueSendToBack(CanTxQueue, TxMessage, 5);
		// @rachel: Make sure this is the behavior you want; I've had trouble
		// with toggling on-and-off the CAN interrupts
        CANx->IER |= CAN_IT_TME;
    }
}

// @rachel: What's the interface that you want a host program to use?
// Do you want programs to be able to register themselves as listeners?
// Do you want to set up filters?
// Do you want to have interrupts that handle filling TX mailboxes?
// It seems like there's some missing infrastructure.