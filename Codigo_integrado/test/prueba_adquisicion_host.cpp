#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "arduino_simulado/Arduino.h"
#include "../src/Sensores/Tension/Tension.h"
#include "../src/Sensores/Corriente/Corriente.h"
#include "../src/Sensores/Energia/SesionEnergia.h"

static uint32_t relojMs = 0, tensionMV = 450, corrienteMV = 2840;
uint32_t millis() { return relojMs; }
uint32_t analogReadMilliVolts(uint8_t pin) {
    return pin == Config::PIN_TENSION ? tensionMV : corrienteMV;
}
uint16_t analogRead(uint8_t pin) {
    return static_cast<uint16_t>(analogReadMilliVolts(pin) * 4095 / 3300);
}
void pinMode(uint8_t, int) {}
void analogReadResolution(uint8_t bits) { assert(bits == 12); }
void analogSetPinAttenuation(uint8_t, adc_attenuation_t) {}
static void avanzar(uint32_t duracionMs) {
    for (uint32_t i = 0; i < duracionMs; ++i) {
        leerTensionCompleta();
        actualizarSensorCorriente();
        ++relojMs;
    }
}
static bool cerca(float a, float b) { return fabsf(a - b) < 0.001f; }

int main() {
    inicializarTension();
    inicializarSensorCorriente(false);
    assert(!sensorCorrienteListo());
    avanzar(1000);
    MedidaTension t = leerTensionCompleta();
    MedidaCorriente c = obtenerMedidaCorriente();
    assert(t.valida && c.valida && !c.calibrando);
    assert(cerca(t.voltajeBateria, 48.0f));
    assert(cerca(c.corrienteA, 8.5f));
    assert(cerca(c.offsetSensorV, 2.500f)); // 2,840 V NO se convierte en cero.

    // Ruta completa: señales ADC -> módulos -> energía durante 60 s reales
    // del reloj de prueba. No se inyectan amperes/volts directamente al cálculo.
    SesionEnergia sesion(Config::CAPACIDAD_PACK_AH, Config::TENSION_NOMINAL_PACK_V);
    sesion.iniciar(relojMs);
    for (uint32_t ms = 0; ms <= 60000; ++ms) {
        t = leerTensionCompleta();
        actualizarSensorCorriente();
        c = obtenerMedidaCorriente();
        sesion.actualizar(t.voltajeBateria, c.corrienteA, relojMs,
                          t.lista && t.valida && c.lista && c.valida);
        ++relojMs;
    }
    assert(cerca(sesion.obtenerEstado().potenciaW, 408.0f));
    assert(cerca(sesion.obtenerEstado().whConsumidos, 6.8f));
    assert(cerca(sesion.obtenerEstado().ahConsumidos, 8.5f / 60.0f));
    sesion.pausar(relojMs);

    tensionMV = 1250; corrienteMV = 2700;
    avanzar(1000);
    assert(cerca(leerTensionCompleta().voltajeBateria, 52.0f));
    assert(cerca(obtenerCorrienteA(), 5.0f));
    tensionMV = 862; // Comprueba que no se redondea a pasos de 25 mV.
    avanzar(1000);
    assert(cerca(leerTensionCompleta().voltajeBateria, 50.06f));

    tensionMV = 0; corrienteMV = 3300;
    avanzar(1000);
    assert(!leerTensionCompleta().valida && !sensorCorrienteListo());
    tensionMV = 850; corrienteMV = 2540;
    inicializarSensorCorriente(true);
    avanzar(Config::ESTABILIZACION_CORRIENTE_MS - 1);
    assert(obtenerMedidaCorriente().calibrando && !sensorCorrienteListo());
    avanzar(Config::MUESTRAS_CALIBRACION * Config::INTERVALO_CALIBRACION_MS + 200);
    c = obtenerMedidaCorriente();
    assert(!c.calibrando && c.valida);
    assert(cerca(c.offsetSensorV, 2.540f) && cerca(c.corrienteA, 0.0f));
    corrienteMV = 2880;
    avanzar(1000);
    assert(cerca(obtenerCorrienteA(), 8.5f));
    printf("ADC OK: ruta completa de 60 s = 6.8 Wh; fuentes variables, fuera de escala y calibracion.\n");
    return 0;
}
