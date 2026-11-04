#pragma once
#include "FreeRTOS.h"
#include <stddef.h>
struct ColaFake;
using QueueHandle_t=ColaFake*;
QueueHandle_t xQueueCreate(UBaseType_t,UBaseType_t);
BaseType_t xQueueSend(QueueHandle_t,const void*,TickType_t);
BaseType_t xQueueReceive(QueueHandle_t,void*,TickType_t);
UBaseType_t uxQueueMessagesWaiting(QueueHandle_t);
