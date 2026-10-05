#include "TareaDisplay.h"
#include <math.h>
#include "../Rtos/Rtos.h"
#include "../Rtos/ColaDisplay.h"
#include "../Display/Display_COM11441.h"

namespace {
void tarea(void*) {
    DisplayCOM11441 display(Serial1, Config::PIN_TX_DISPLAY, Config::PIN_RX_DISPLAY);
    // Permite arrancar al display sin detener la adquisición en otras tareas.
    vTaskDelay(pdMS_TO_TICKS(Config::ESTABILIZACION_DISPLAY_MS));
    display.begin(Config::BAUDIOS_DISPLAY);
    display.clear();
    display.setBrightness(Config::BRILLO_DISPLAY);
    display.showText("----");
    logSerial("Display UART1: TX GPIO%u, RX GPIO%u, %lu baudios.\n",
              Config::PIN_TX_DISPLAY, Config::PIN_RX_DISPLAY,
              static_cast<unsigned long>(Config::BAUDIOS_DISPLAY));

    bool mostrabaNumero = false;
    int ultimoNumero = 0;
    for (;;) {
        EstadoDisplay estado = {};
        // Espera dormida: no consulta ADC, no lee Serial USB ni recalcula SOC.
        if (!recibirEstadoDisplay(estado, portMAX_DELAY)) continue;
        const bool hayNumero = estado.modoPrueba ||
            (estado.lecturaDisponible && isfinite(estado.porcentajeBateria));
        if (!hayNumero) {
            if (mostrabaNumero) display.showText("----");
            mostrabaNumero = false;
            continue;
        }
        int numero;
        if (estado.modoPrueba) {
            numero = estado.numeroPrueba;
            if (numero > 9999) numero = 9999;
            else if (numero < -999) numero = -999;
        } else {
            float soc = estado.porcentajeBateria;
            if (soc < 0.0f) soc = 0.0f;
            else if (soc > 100.0f) soc = 100.0f;
            numero = static_cast<int>(soc + 0.5f); // Redondeo al entero cercano.
        }
        if (!mostrabaNumero || numero != ultimoNumero) {
            if (estado.modoPrueba) display.showNumber(numero);
            else display.showBatteryPercentage(numero);
            ultimoNumero = numero;
            mostrabaNumero = true;
        }
    }
}
} // namespace

bool crearTareaDisplay() {
    if (!HABILITAR_TAREA_DISPLAY) return true;
    return xTaskCreatePinnedToCore(tarea, "Display", STACK_DISPLAY, nullptr,
        PRIO_DISPLAY, nullptr, CORE_0) == pdPASS;
}
