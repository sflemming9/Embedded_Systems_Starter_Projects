#pragma once

// Make sure everything that depends on IS_FRONT includes "global.h"
//#define IS_FRONT 1
//#define IS_FRONT_DERP 1
#define IS_REAR 1

#ifdef IS_FRONT
  #define DEVICE_ID 0x03
  #define DEVICE_NAME "Front lights"
#elif IS_REAR
  #define DEVICE_ID 0x05
  #define DEVICE_NAME "Rear lights"
#endif

// ###################
// ##### M4 CORE #####
// ###################

#define CYCCNT          (*(volatile uint32_t*)0xE0001004)

#define DWT_CTRL        (*(volatile uint32_t*)0xE0001000)
#define DWT_CTRL_CYCEN  0x00000001

#define ITM_TRACE       (*(volatile uint32_t*) 0xE0000E80)

#define SCB_DEMCR		(*(volatile uint32_t*)0xE000EDFC)
#define SCB_DEMCR_TRCEN 0x01000000

// ##########################
// ##### GLOBAL DEFINES #####
// ##########################

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define BOUND(a, b, c)	(MIN(b, MAX(a, c)))

// ####################
// ##### INCLUDES #####
// ####################

// STM32 peripheral library files
#include <stm32f4xx.h>

// ANSI C library files
#include <stdio.h>
#include <string.h>

// Scheduler header files
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

// Local definitions
#include "lis331.h"
#include "l3g4200.h"
#include "ms5803.h"
#include "config.h"
#include "catalog.h"
#include "canrouter.h"

// ############################
// ##### GLOBAL VARIABLES #####
// ############################

extern volatile int64_t time_ms;
// Outgoing can messages
extern xQueueHandle CanTxQueue;
// All incoming messages
extern xQueueHandle CanRxQueue;
extern Can mainCan;

// #############################
// ##### CATALOG VARIABLES #####
// #############################

// Incoming catalog messages should go here.
extern xQueueHandle CatalogRxQueue;

extern Catalog cat;

#define CAN_TXLEN		128
