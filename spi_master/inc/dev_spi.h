#pragma once

#include <stm32f4xx.h>
#include <stdbool.h>
#include "spi.h"

extern Spi demo_spi;

void dev_spiTask(void *pvParameters);