#ifndef RTOS_H
#define RTOS_H

#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>
#include "../Configuracion.h"

// Habilitación por tarea (no se toca Configuracion.h).
constexpr bool HABILITAR_TAREA_CORRIENTE   = true;
constexpr bool HABILITAR_TAREA_ENERGIA     = true;
constexpr bool HABILITAR_TAREA_PCNT        = true;
constexpr bool HABILITAR_TAREA_GPS         = true;
constexpr bool HABILITAR_TAREA_TEMPERATURA = false;
constexpr bool HABILITAR_TAREA_DISPLAY     = true;

// Latido de los stubs (Etapa 2); las etapas siguientes lo reemplazan por lógica real.
constexpr bool HEARTBEAT_TAREAS = true;
constexpr uint32_t PERIODO_HEARTBEAT_MS = 5000;

// Salida de diagnóstico de la base de tiempo PCNT en GPIO12 (pin de strapping: false la desactiva).
constexpr bool PCNT_SALIDA_DIAGNOSTICO = false;

// Regla heredada de main PCNT: la temperatura solo se mide mientras no hubo primer pulso
// (vehículo detenido). false = medir siempre.
constexpr bool TEMPERATURA_SOLO_SIN_PULSO = true;

// Núcleos (tick = 1 ms).
constexpr BaseType_t CORE_0 = 0;
constexpr BaseType_t CORE_1 = 1;

// Prioridades.
constexpr UBaseType_t PRIO_CORRIENTE   = 4;
constexpr UBaseType_t PRIO_PCNT        = 3;
constexpr UBaseType_t PRIO_GPS         = 3;
constexpr UBaseType_t PRIO_ENERGIA     = 2;
constexpr UBaseType_t PRIO_TEMPERATURA = 1;
constexpr UBaseType_t PRIO_DISPLAY     = 1;

// Stacks (bytes en Arduino-ESP32).
constexpr uint32_t STACK_CORRIENTE   = 3072;
constexpr uint32_t STACK_PCNT        = 4096;
constexpr uint32_t STACK_ENERGIA     = 4096;
constexpr uint32_t STACK_GPS         = 4096;
constexpr uint32_t STACK_TEMPERATURA = 3072;
constexpr uint32_t STACK_DISPLAY     = 3072;

// Periodos (ms).
constexpr uint32_t PERIODO_CORRIENTE_MS   = Config::INTERVALO_CORRIENTE_MS;
constexpr uint32_t PERIODO_PCNT_MS        = 100;
constexpr uint32_t PERIODO_ENERGIA_MS     = 20;
constexpr uint32_t PERIODO_GPS_MS         = 20;
constexpr uint32_t PERIODO_TEMPERATURA_MS = 1000;

// Serial TX protegido con mutex (creado en main.cpp antes de las tareas).
extern SemaphoreHandle_t mtxSerial;

inline void logSerial(const char* fmt, ...) {
    char buf[192];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (mtxSerial && xSemaphoreTake(mtxSerial, pdMS_TO_TICKS(500)) == pdTRUE) {
        Serial.print(buf);
        xSemaphoreGive(mtxSerial);
    }
}

// Bloqueo RAII del Serial para varias líneas seguidas (no llamar logSerial adentro).
struct BloqueoSerial {
    bool ok;
    BloqueoSerial() : ok(mtxSerial && xSemaphoreTake(mtxSerial, pdMS_TO_TICKS(1000)) == pdTRUE) {}
    ~BloqueoSerial() { if (ok) xSemaphoreGive(mtxSerial); }
};

#endif
