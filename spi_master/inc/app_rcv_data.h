#pragma once

#include <stm32f4xx.h>
#include <stdbool.h>

bool drive_state = false;

void app_rcv_dataTask(void *pvParameters);