#include <stm32f4xx_it.h>
#include "global.h"

void NMI_Handler(void){}

void HardFault_Handler(void){while (1);}

void MemManage_Handler(void){while (1);}

void BusFault_Handler(void){while (1);}

void UsageFault_Handler(void){while(1);}

void DebugMon_Handler(void);

// ###############
// ##### CAN #####
// ###############

// Receive CAN messages from the hardware and enqueue them.
// Don't wait for the queue to have an empty slot - let it overflow.
void CAN1_RX0_IRQHandler(void){
    CAN_RxIsr(&mainCan, CAN_FIFO0);
}

// Take messages from the transmit queue and send them out.
// Turn off the TX interrupt if we run out of messages in the queue.

void CAN1_TX_IRQHandler(void){
    CAN_TxIsr(&mainCan);
}