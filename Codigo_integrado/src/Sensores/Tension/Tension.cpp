#include "Tension.h"
#include <Arduino.h>

namespace {
MedidaTension resultado = {};
uint32_t ultimoMuestreoMs = 0;
uint32_t sumaMiliboltios = 0, sumaAdc = 0;
uint8_t contador = 0;
bool configuracionValida = false;
}

void inicializarTension() {
    configuracionValida = Config::errorConfiguracion() == nullptr;
    resultado = {};
    sumaMiliboltios = sumaAdc = 0;
    contador = 0;
    ultimoMuestreoMs = millis() - Config::INTERVALO_TENSION_MS;
    if (!configuracionValida) return;
    pinMode(Config::PIN_TENSION, INPUT);
    analogReadResolution(Config::RESOLUCION_ADC_BITS);
    analogSetPinAttenuation(Config::PIN_TENSION, ADC_11db);
}

MedidaTension leerTensionCompleta() {
    if (!configuracionValida) return resultado;
    const uint32_t ahoraMs = millis();
    if (ahoraMs - ultimoMuestreoMs < Config::INTERVALO_TENSION_MS) return resultado;
    ultimoMuestreoMs = ahoraMs;
    const uint32_t milivoltios = analogReadMilliVolts(Config::PIN_TENSION);
    const uint16_t adc = analogRead(Config::PIN_TENSION);
    // El ADC crudo se conserva solo como dato de diagnóstico/compatibilidad.
    // No se usa para rechazar ni bloquear la conversión en milivoltios.
    sumaMiliboltios += milivoltios;
    sumaAdc += adc;
    if (++contador < Config::MUESTRAS_TENSION) return resultado;

    resultado.voltajePinV = sumaMiliboltios /
        (1000.0f * Config::MUESTRAS_TENSION);
    resultado.adc = resultado.adc_crudo = sumaAdc / Config::MUESTRAS_TENSION;
    resultado.ultimaLecturaMs = ahoraMs;
    resultado.lista = true;
    resultado.voltaje = resultado.voltajePinV;
    resultado.voltajeBateria = tensionPackDesdeVoltajePin(resultado.voltajePinV);
    // Indicador antiguo de escala: no es el SOC calculado desde Ah.
    resultado.porcentaje = 100.0f *
        (resultado.voltajePinV - Config::ENTRADA_TENSION_MIN_V) /
        (Config::ENTRADA_TENSION_MAX_V - Config::ENTRADA_TENSION_MIN_V);
    contador = 0;
    sumaMiliboltios = sumaAdc = 0;
    return resultado;
}
