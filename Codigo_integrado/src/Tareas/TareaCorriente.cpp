#include "TareaCorriente.h"
#include "ModoEnergia.h"
#include "../Rtos/Rtos.h"
#include "../Rtos/Compartidos.h"
#include "../Sensores/Corriente/Corriente.h"

// Reemplaza a "main Sensor Corriente.cpp" (API vieja, ESP32-C3): la recalibración del
// offset que antes se pedía con 'c' por Serial ahora es el comando "calibrar" (tarea
// Energía), que levanta la bandera que consume esta tarea.
namespace {
void tarea(void*) {
    if (Config::errorConfiguracion() || !usaCorrienteADC()) vTaskSuspend(NULL); // El error lo informa Energía.
    inicializarSensorCorriente(calibraAlArrancar());

    TickType_t ultimo = xTaskGetTickCount();
    uint32_t ultimoLatidoMs = millis();
    for (;;) {
        // Corriente (prio 4) no es desalojada por Energía (prio 2) en el mismo núcleo:
        // tras recalibrar publica enseguida calibrando=true, sin ventana con datos viejos.
        if (consumirSolicitudCalibracion()) recalibrarSensorCorriente();
        actualizarSensorCorriente();
        publicarCorriente(obtenerMedidaCorriente());
        if (HEARTBEAT_TAREAS && millis() - ultimoLatidoMs >= PERIODO_HEARTBEAT_MS) {
            ultimoLatidoMs = millis();
            logSerial("[Corriente] latido core=%d stack_libre=%u\n", xPortGetCoreID(),
                      (unsigned)uxTaskGetStackHighWaterMark(NULL));
        }
        vTaskDelayUntil(&ultimo, pdMS_TO_TICKS(PERIODO_CORRIENTE_MS));
    }
}
} // namespace

bool crearTareaCorriente() {
    if (!HABILITAR_TAREA_CORRIENTE) return true;
    return xTaskCreatePinnedToCore(tarea, "Corriente", STACK_CORRIENTE, nullptr, PRIO_CORRIENTE, nullptr, CORE_1) == pdPASS;
}
