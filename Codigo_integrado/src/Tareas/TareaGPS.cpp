#include "TareaGPS.h"
#include "../Rtos/Rtos.h"
#include "../Rtos/Compartidos.h"
#include "../Sensores/GPS/GPS.h"

// Portado de "main GPS.cpp". Decisión: se descarta SerialUART1 (RX=13/TX=14), que en el
// original estaba comentado como ejemplo y sin dispositivo real asociado.
// Serial2 (GPIO16/17) lo usa solo esta tarea; el resto lee el snapshot publicado.
namespace {
void imprimirGPS(const DatosGPS& g) {
    BloqueoSerial b;
    if (!b.ok) return;
    Serial.print("LAT: ");
    Serial.print(g.latitud, 6);
    Serial.print(" | LONG: ");
    Serial.println(g.longitud, 6);

    Serial.print("SPEED: ");
    Serial.print(g.velocidadKmH, 2);
    Serial.print(" km/h | RUMBO: ");
    if (g.rumboValido) {
        Serial.print(g.rumboGrados, 1);
        Serial.println("°");
    } else {
        Serial.println("Sin rumbo");
    }

    Serial.print("HDOP: ");
    Serial.print(g.hdop);
    Serial.print(" | Satélites: ");
    Serial.println(g.satelites);

    char bufferFechaHora[40];
    snprintf(bufferFechaHora, sizeof(bufferFechaHora), "HORA ARG: %04d/%02d/%02d %02d:%02d:%02d",
             g.anio, g.mes, g.dia, g.hora, g.minuto, g.segundo);
    Serial.println(bufferFechaHora);
    Serial.println("----------------------------------------");
}

void tarea(void*) {
    inicializarGPS(); // Bloquea ~2 s: solo esta tarea espera.

    TickType_t ultimo = xTaskGetTickCount();
    uint32_t ultimoLatidoMs = millis();
    for (;;) {
        actualizarGPS(); // Drena Serial2 sin bloquear.
        if (datosGPSActualizados()) {
            const DatosGPS g = obtenerDatosGPS();
            publicarGPS(g);
            imprimirGPS(g);
        }
        if (HEARTBEAT_TAREAS && millis() - ultimoLatidoMs >= PERIODO_HEARTBEAT_MS) {
            ultimoLatidoMs = millis();
            logSerial("[GPS] latido core=%d stack_libre=%u\n", xPortGetCoreID(),
                      (unsigned)uxTaskGetStackHighWaterMark(NULL));
        }
        vTaskDelayUntil(&ultimo, pdMS_TO_TICKS(PERIODO_GPS_MS));
    }
}
} // namespace

bool crearTareaGPS() {
    if (!HABILITAR_TAREA_GPS) return true;
    return xTaskCreatePinnedToCore(tarea, "GPS", STACK_GPS, nullptr, PRIO_GPS, nullptr, CORE_0) == pdPASS;
}
