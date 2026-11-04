#pragma once
#include <stdint.h>
using BaseType_t=int; using UBaseType_t=unsigned; using TickType_t=uint32_t;
using SemaphoreHandle_t=void*; using TaskHandle_t=void*;
constexpr BaseType_t pdTRUE=1,pdPASS=1;
constexpr TickType_t portMAX_DELAY=UINT32_MAX;
#define pdMS_TO_TICKS(x) (static_cast<TickType_t>(x))
SemaphoreHandle_t xSemaphoreCreateMutex();
BaseType_t xSemaphoreTake(SemaphoreHandle_t,TickType_t);
BaseType_t xSemaphoreGive(SemaphoreHandle_t);
BaseType_t xTaskCreatePinnedToCore(void(*)(void*),const char*,uint32_t,void*,UBaseType_t,TaskHandle_t*,BaseType_t);
void vTaskDelay(TickType_t);
void vTaskDelayUntil(TickType_t*,TickType_t);
TickType_t xTaskGetTickCount();
UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t);
