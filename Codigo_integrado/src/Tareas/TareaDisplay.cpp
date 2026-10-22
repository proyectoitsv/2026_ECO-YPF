#include "TareaDisplay.h"
#include <math.h>
#include "../Rtos/Rtos.h"
#include "../Rtos/ColaDisplay.h"
#include "../Display/Display_COM11441.h"

namespace {
// Tiempos editables. Solo afectan la visibilidad del COM-11441.
constexpr uint32_t TIEMPO_ENCENDIDO_MS = 2000;
constexpr uint32_t TIEMPO_APAGADO_MS = 3000;
constexpr TickType_t TIEMPO_ENCENDIDO_TICKS = pdMS_TO_TICKS(TIEMPO_ENCENDIDO_MS);
constexpr TickType_t TIEMPO_CICLO_TICKS =
    TIEMPO_ENCENDIDO_TICKS + pdMS_TO_TICKS(TIEMPO_APAGADO_MS);
static_assert(TIEMPO_ENCENDIDO_TICKS > 0 &&
              TIEMPO_CICLO_TICKS > TIEMPO_ENCENDIDO_TICKS,
              "Los tiempos del display deben ocupar al menos un tick.");

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
    bool estabaEncendido = false;
    bool hayContenidoMostrado = false;
    bool mostrabaNumero = false;
    int ultimoNumero = 0;
    TickType_t inicioCiclo = xTaskGetTickCount();

    for (;;) {
        const TickType_t ahora = xTaskGetTickCount();
        // La resta sin signo admite el desbordamiento del contador de ticks.
        TickType_t transcurrido = ahora - inicioCiclo;
        if (transcurrido >= TIEMPO_CICLO_TICKS) {
            // Conserva el periodo de 5 s aunque lleguen datos o haya retrasos.
            inicioCiclo += (transcurrido / TIEMPO_CICLO_TICKS) * TIEMPO_CICLO_TICKS;
            transcurrido = ahora - inicioCiclo;
        }

        // La visibilidad depende del reloj del ciclo, no de cuándo llegan
        // capturas CSV o actualizaciones nuevas de SOC.
        const bool encendido = transcurrido < TIEMPO_ENCENDIDO_TICKS;
        if (!encendido) {
            // Una sola orden 0x76 por apagado: todos los LED quedan en blanco.
            // El ATmega sigue alimentado. No se cambia brillo ni EEPROM.
            if (estabaEncendido) display.clear();
            hayContenidoMostrado = false;
            mostrabaNumero = false;
        } else {
            const bool hayNumero = estado.modoPrueba ||
                (estado.lecturaDisponible && isfinite(estado.porcentajeBateria));
            if (!hayNumero) {
                if (!hayContenidoMostrado || mostrabaNumero) display.showText("----");
                mostrabaNumero = false;
            } else {
                int numero;
                if (estado.modoPrueba) {
                    numero = estado.numeroPrueba;
                    if (numero > 9999) numero = 9999;
                    else if (numero < -999) numero = -999;
                } else {
                    float soc = estado.porcentajeBateria;
                    if (soc < 0.0f) soc = 0.0f;
                    else if (soc > 100.0f) soc = 100.0f;
                    numero = static_cast<int>(soc + 0.5f); // Mismo redondeo anterior.
                }
                // Al volver del apagado se reenvia, aunque el numero sea igual.
                if (!hayContenidoMostrado || !mostrabaNumero || numero != ultimoNumero) {
                    if (estado.modoPrueba) display.showNumber(numero);
                    else display.showBatteryPercentage(numero);
                    ultimoNumero = numero;
                }
                mostrabaNumero = true;
            }
            hayContenidoMostrado = true;
        }
        estabaEncendido = encendido;

        // Espera hasta un dato nuevo O el proximo cambio de fase.
        // FreeRTOS duerme esta tarea; las otras tareas siguen ejecutandose.
        const TickType_t finFase = encendido ?
            TIEMPO_ENCENDIDO_TICKS : TIEMPO_CICLO_TICKS;
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
