#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "arduino_simulado/Arduino.h"
#include "../src/Sensores/Tension/Tension.h"
#include "../src/Sensores/Corriente/Corriente.h"
#include "../src/Sensores/Energia/SesionEnergia.h"

static uint32_t relojMs = 0, tensionMV = 450, corrienteMV = 2840;
static bool adcCrudoEnMaximo = false;
uint32_t millis() { return relojMs; }
uint32_t analogReadMilliVolts(uint8_t pin) {
    return pin == Config::PIN_TENSION ? tensionMV : corrienteMV;
}
uint16_t analogRead(uint8_t pin) {
    if (adcCrudoEnMaximo) return 4095;
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
    assert(t.lista && c.lista && !c.calibrando);
    assert(cerca(t.voltajeBateria, 48.0f));
    assert(cerca(c.corrienteA, 8.5f));
    assert(cerca(c.offsetSensorV, 2.500f)); // 2,840 V NO se convierte en cero.

    // ADC crudo en máximo: no debe rechazar la conversión en milivoltios.
    // Ruta completa: señales ADC -> módulos -> energía durante 60 s reales
    // del reloj de prueba. No se inyectan amperes/volts directamente al cálculo.
    SesionEnergia sesion(Config::CAPACIDAD_PACK_AH, Config::TENSION_NOMINAL_PACK_V);
    adcCrudoEnMaximo = true;
    sesion.iniciar(relojMs);
    for (uint32_t ms = 0; ms <= 60000; ++ms) {
        t = leerTensionCompleta();
        actualizarSensorCorriente();
        c = obtenerMedidaCorriente();
        sesion.actualizar(t.voltajeBateria, c.corrienteA, relojMs,
                          t.lista && c.lista && !c.calibrando);
        ++relojMs;
    }
    assert(cerca(sesion.obtenerEstado().potenciaW, 408.0f));
    assert(cerca(sesion.obtenerEstado().whConsumidos, 6.8f));
    assert(cerca(sesion.obtenerEstado().ahConsumidos, 8.5f / 60.0f));
    assert(fabs(sesion.segundosIntegrados() - 60.0) < 0.00001);
    sesion.pausar(relojMs);
    adcCrudoEnMaximo = false;

    tensionMV = 1250; corrienteMV = 2700;
    avanzar(1000);
    assert(cerca(leerTensionCompleta().voltajeBateria, 52.0f));
    assert(cerca(obtenerCorrienteA(), 5.0f));
    tensionMV = 862; // Comprueba que no se redondea a pasos de 25 mV.
    avanzar(1000);
    assert(cerca(leerTensionCompleta().voltajeBateria, 50.06f));

    // Los puntos de calibración no son límites: se convierte sin recortar.
    tensionMV = 1270; corrienteMV = 3300;
    avanzar(1000);
    assert(leerTensionCompleta().lista && sensorCorrienteListo());
    assert(cerca(leerTensionCompleta().voltajeBateria, 52.1f));
    assert(cerca(obtenerCorrienteA(), 20.0f));
    tensionMV = 0; corrienteMV = 0;
    avanzar(1000);
    assert(leerTensionCompleta().lista && sensorCorrienteListo());
    assert(cerca(leerTensionCompleta().voltajeBateria, 45.75f));
    assert(cerca(obtenerCorrienteA(), -62.5f));

    // Todas las muestras aportan al promedio, incluida una muestra distinta.
    // Con el filtro recortado anterior, esta muestra se habría descartado.
    corrienteMV = 2840;
    inicializarSensorCorriente(false);
    for (uint8_t i = 0; i < Config::MUESTRAS_CORRIENTE; ++i) {
        corrienteMV = i + 1 == Config::MUESTRAS_CORRIENTE ? 3000 : 2840;
        const bool terminado = actualizarSensorCorriente();
        assert(terminado == (i + 1 == Config::MUESTRAS_CORRIENTE));
        relojMs += Config::INTERVALO_CORRIENTE_MS;
    }
    const float esperadoPinV =
        ((Config::MUESTRAS_CORRIENTE - 1) * 2840.0f + 3000.0f) /
        (1000.0f * Config::MUESTRAS_CORRIENTE);
    assert(cerca(obtenerMedidaCorriente().voltajePinV, esperadoPinV));
    assert(cerca(obtenerCorrienteA(),
                 corrienteDesdeVoltajeSensor(esperadoPinV, 2.500f)));

    tensionMV = 850; corrienteMV = 2540;
    inicializarSensorCorriente(true);
    avanzar(Config::ESTABILIZACION_CORRIENTE_MS - 1);
    assert(obtenerMedidaCorriente().calibrando && !sensorCorrienteListo());
    avanzar(Config::MUESTRAS_CALIBRACION * Config::INTERVALO_CALIBRACION_MS + 200);
    c = obtenerMedidaCorriente();
    assert(!c.calibrando && c.lista);
    assert(cerca(c.offsetSensorV, 2.540f) && cerca(c.corrienteA, 0.0f));
    corrienteMV = 2880;
    avanzar(1000);
    assert(cerca(obtenerCorrienteA(), 8.5f));
    printf("ADC OK: 60 s = 6.8 Wh sin cortes; promedio simple, conversion sin recortes y calibracion.\n");
    return 0;
}
