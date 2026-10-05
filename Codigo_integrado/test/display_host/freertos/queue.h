#ifndef QUEUE_DISPLAY_HOST_H
#define QUEUE_DISPLAY_HOST_H

#include <stddef.h>
#include "FreeRTOS.h"
struct ColaSimulada;
using QueueHandle_t = ColaSimulada*;
QueueHandle_t xQueueCreate(UBaseType_t, UBaseType_t);
BaseType_t xQueueOverwrite(QueueHandle_t, const void*);
BaseType_t xQueueReceive(QueueHandle_t, void*, TickType_t);

#endif
