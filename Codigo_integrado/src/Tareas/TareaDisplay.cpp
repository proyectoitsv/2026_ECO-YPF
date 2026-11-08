#include "TareaDisplay.h"
#include <math.h>
#include "../Rtos/Rtos.h"
#include "../Rtos/ColaDisplay.h"
#include "../Display/Display_COM11441.h"

namespace {
// Vacío 3 s, velocidad 2 s, vacío 3 s, velocidad 2 s,
// vacío 3 s, batería 3 s. La vuelta completa dura 16 segundos.
constexpr TickType_t FINES_FASE[] = {
    pdMS_TO_TICKS(3000), pdMS_TO_TICKS(5000), pdMS_TO_TICKS(8000),
    pdMS_TO_TICKS(10000), pdMS_TO_TICKS(13000), pdMS_TO_TICKS(16000)
};
constexpr TickType_t TIEMPO_CICLO_TICKS = FINES_FASE[5];

void tarea(void*) {
    DisplayCOM11441 display(Serial1, Config::PIN_TX_DISPLAY, Config::PIN_RX_DISPLAY);
    // Permite arrancar al display sin detener la adquisición en otras tareas.
    vTaskDelay(pdMS_TO_TICKS(Config::ESTABILIZACION_DISPLAY_MS));
    display.begin(Config::BAUDIOS_DISPLAY);
    display.clear();
    display.setBrightness(Config::BRILLO_DISPLAY);
    logSerial("Display UART1: TX GPIO%u, RX GPIO%u, %lu baudios.\n",
              Config::PIN_TX_DISPLAY, Config::PIN_RX_DISPLAY,
              static_cast<unsigned long>(Config::BAUDIOS_DISPLAY));

    EstadoDisplay estado = {};
    // Tipos de contenido: 0 vacío, 1 sin dato, 2 prueba, 3 velocidad, 4 batería.
    int ultimoTipo = 0;
    int ultimoNumero = 0;
    TickType_t inicioCiclo = xTaskGetTickCount();

    for (;;) {
        const TickType_t ahora = xTaskGetTickCount();
        // La resta sin signo admite el desbordamiento del contador de ticks.
        TickType_t transcurrido = ahora - inicioCiclo;
        if (transcurrido >= TIEMPO_CICLO_TICKS) {
            inicioCiclo += (transcurrido / TIEMPO_CICLO_TICKS) * TIEMPO_CICLO_TICKS;
            transcurrido = ahora - inicioCiclo;
        }
        unsigned fase = 0;
        while (transcurrido >= FINES_FASE[fase]) ++fase;
        const bool encendido = (fase % 2) != 0;
        int tipo = 0;
        int numero = 0;
        if (encendido) {
            tipo = 1;
            if (estado.modoPrueba) {
                tipo = 2;
                numero = estado.numeroPrueba;
                if (numero > 9999) numero = 9999;
                else if (numero < -999) numero = -999;
            } else if (fase == 5) {
                if (estado.lecturaDisponible && isfinite(estado.porcentajeBateria)) {
                    tipo = 4;
                    float soc = estado.porcentajeBateria;
                    if (soc < 0.0f) soc = 0.0f;
                    else if (soc > 100.0f) soc = 100.0f;
                    numero = static_cast<int>(soc + 0.5f);
                }
            } else if (estado.velocidadDisponible && isfinite(estado.velocidadKmH) &&
                       estado.velocidadKmH >= 0.0f && estado.velocidadKmH <= 99.9f) {
                tipo = 3;
                numero = static_cast<int>(estado.velocidadKmH * 10.0f + 0.5f);
            }
        }
        // Solo se transmite al cambiar el dato visible o al cambiar de fase.
        // Los datos nuevos actualizan la velocidad sin alargar los dos segundos.
        if (tipo != ultimoTipo || (tipo >= 2 && numero != ultimoNumero)) {
            if (tipo == 0) display.clear();
            else if (tipo == 1) display.showText("----");
            else if (tipo == 2) display.showNumber(numero);
            else if (tipo == 3) display.showSpeedTenths(numero);
            else display.showBatteryPercentage(numero);
            ultimoTipo = tipo;
            ultimoNumero = numero;
        }

        // Espera hasta un dato nuevo O el proximo cambio de fase.
        // FreeRTOS duerme esta tarea; las otras tareas siguen ejecutandose.
        const TickType_t finFase = FINES_FASE[fase];
        const TickType_t transcurridoAlEsperar = xTaskGetTickCount() - inicioCiclo;
        if (transcurridoAlEsperar >= finFase) continue;
        const TickType_t espera = finFase - transcurridoAlEsperar;
        EstadoDisplay nuevoEstado = {};
        if (recibirEstadoDisplay(nuevoEstado, espera)) estado = nuevoEstado;
        // Tambien durante los 3 s apagado se conserva el ultimo estado recibido.
    }
}
} // namespace

bool crearTareaDisplay() {
    if (!HABILITAR_TAREA_DISPLAY) return true;
    return xTaskCreatePinnedToCore(tarea, "Display", STACK_DISPLAY, nullptr,
        PRIO_DISPLAY, nullptr, CORE_0) == pdPASS;
}
