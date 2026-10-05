// Firmware integrado ECO-YPF: cada sketch original es una tarea FreeRTOS.
//
// Tarea        Origen                              Core Prio Periodo  Stack
// Corriente    Sensor Corriente + parte de Energia  1    4    5 ms     3072
// PCNT         main PCNT.cpp (velocidad)            1    3    100 ms   4096
// Energía      main tension.cpp (vigente)           1    2    20 ms    4096
// GPS          main GPS.cpp                         0    3    20 ms    4096
// Temperatura  GPS/PCNT/Energia (DS18B20 25 y 33)   0    1    1000 ms  3072
// Display      SOC recibido por cola / UART1       0    1    espera   3072
//
// Constantes (flags HABILITAR_*, prioridades, stacks, periodos) en Rtos/Rtos.h.
// Datos entre tareas: snapshots con mutex en Rtos/Compartidos.h.
// Serial: RX solo en Energía; TX por logSerial()/BloqueoSerial (mtxSerial).
// Los main *.cpp originales quedan como referencia y se excluyen en platformio.ini.
#include <Arduino.h>
#include "Rtos/Rtos.h"
#include "Rtos/Compartidos.h"
#include "Rtos/ColaDisplay.h"
#include "Tareas/TareaCorriente.h"
#include "Tareas/TareaPCNT.h"
#include "Tareas/TareaEnergia.h"
#include "Tareas/TareaGPS.h"
#include "Tareas/TareaTemperatura.h"
#include "Tareas/TareaDisplay.h"

SemaphoreHandle_t mtxSerial = nullptr;

[[noreturn]] static void fallaCritica(const char* msg) {
    Serial.printf("ERROR: %s. Reiniciando...\n", msg);
    Serial.flush();
    delay(1000);
    ESP.restart();
    for (;;) delay(1000);
}

void setup() {
    Serial.begin(Config::BAUDIOS_MONITOR);
    mtxSerial = xSemaphoreCreateMutex();
    if (!mtxSerial) fallaCritica("no se pudo crear mtxSerial");
    if (!inicializarCompartidos()) fallaCritica("no se pudo crear mtxDatos");
    if (HABILITAR_TAREA_DISPLAY && !inicializarColaDisplay())
        fallaCritica("no se pudo crear la cola del display");

    logSerial("ECO-YPF integrado: arranque\n");

    struct { const char* nombre; bool (*crear)(); } tareas[] = {
        {"Corriente",   crearTareaCorriente},
        {"PCNT",        crearTareaPCNT},
        {"Energia",     crearTareaEnergia},
        {"GPS",         crearTareaGPS},
        {"Temperatura", crearTareaTemperatura},
        {"Display",     crearTareaDisplay},
    };
    for (auto& t : tareas)
        if (!t.crear()) {
            Serial.printf("ERROR: no se pudo crear la tarea %s\n", t.nombre);
            fallaCritica("falla al crear tareas (memoria insuficiente)");
        }
}

void loop() {
    // Arduino ejecuta loop() en su propia tarea; no hace falta, se elimina.
    vTaskDelete(NULL);
}
