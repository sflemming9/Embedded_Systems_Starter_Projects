#include <stm32f4xx_it.h>
#include "bsp.h"

void NMI_Handler(void){}

void HardFault_Handler(void){while (1);}

void MemManage_Handler(void){while (1);}

void BusFault_Handler(void){while (1);}

void UsageFault_Handler(void){while(1);}

void DebugMon_Handler(void);

// Send CAN1 RX FIFO 0 interrupts to the canrouter
void CAN1_RX0_IRQHandler(void){
    printf("interrupt handler\n");
    CAN_RxIsr(&mainCan, 0);
}

// Send CAN1 RX FIFO 1 interrupts to the canrouter
void CAN1_RX1_IRQHandler(void){
    CAN_RxIsr(&mainCan, 1);
}

// Send CAN1 TX interrupts to the canrouter
void CAN1_TX_IRQHandler(void){
    CAN_TxIsr(&mainCan);
}