#include "global.h"
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <pin.h>
#include <catalog.h>

void LightsUpdateTask(void *data);
void LightsCatVarInit(Catalog *cat);