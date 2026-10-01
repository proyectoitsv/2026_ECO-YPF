#include "TareaTemperatura.h"
#include "../Rtos/Rtos.h"
#include "../Rtos/Compartidos.h"
#include "../Sensores/Temperatura/Temperatura.h"

// Única instancia de los dos DS18B20 (GPIO25 y GPIO33), antes duplicados en 3 sketches.
// requestTemperatures() bloquea ~750 ms: solo detiene esta tarea.
namespace {
constexpr float TEMP_DESCONECTADO_C = -127.0f; // DEVICE_DISCONNECTED_C

void tarea(void*) {
    GestorTemperatura sensorTemp1(25);
    GestorTemperatura sensorTemp2(33);
    sensorTemp1.inicializar();
    sensorTemp2.inicializar();

    TickType_t ultimo = xTaskGetTickCount();
    uint32_t ultimoLatidoMs = millis();
    for (;;) {
        if (!TEMPERATURA_SOLO_SIN_PULSO || !leerPrimerPulso()) {
            sensorTemp1.solicitarTemperaturas();
            sensorTemp2.solicitarTemperaturas();
            const float temp1 = sensorTemp1.leerTemperatura(0);
            const float temp2 = sensorTemp2.leerTemperatura(0);
            const float promedio = (temp1 + temp2) / 2.0f;
            const bool valida = temp1 > TEMP_DESCONECTADO_C && temp2 > TEMP_DESCONECTADO_C;
            publicarTemperatura({temp1, temp2, promedio, valida});
            logSerial("Temperatura promedio: %.2f C%s\n", promedio, valida ? "" : " (sensor sin respuesta)");
        }
        if (HEARTBEAT_TAREAS && millis() - ultimoLatidoMs >= PERIODO_HEARTBEAT_MS) {
            ultimoLatidoMs = millis();
            logSerial("[Temperatura] latido core=%d stack_libre=%u\n", xPortGetCoreID(),
                      (unsigned)uxTaskGetStackHighWaterMark(NULL));
        }
        vTaskDelayUntil(&ultimo, pdMS_TO_TICKS(PERIODO_TEMPERATURA_MS));
    }
}
} // namespace

bool crearTareaTemperatura() {
    if (!HABILITAR_TAREA_TEMPERATURA) return true;
    return xTaskCreatePinnedToCore(tarea, "Temperatura", STACK_TEMPERATURA, nullptr, PRIO_TEMPERATURA, nullptr, CORE_0) == pdPASS;
}
