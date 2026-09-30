#include "Tension.h"
#include <Arduino.h>
#include <math.h>

namespace {
MedidaTension resultado = {};
uint32_t ultimoMuestreoMs = 0;
uint32_t sumaMiliboltios = 0, sumaAdc = 0;
uint8_t contador = 0;
bool grupoValido = true, configuracionValida = false;
}

void inicializarTension() {
    configuracionValida = Config::errorConfiguracion() == nullptr;
    resultado = {};
    sumaMiliboltios = sumaAdc = 0;
    contador = 0;
    grupoValido = true;
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
    const float pinV = milivoltios / 1000.0f;
    const bool muestraValida =
        pinV >= Config::ENTRADA_TENSION_MIN_V - Config::TOLERANCIA_TENSION_PIN_V &&
        pinV <= Config::ENTRADA_TENSION_MAX_V + Config::TOLERANCIA_TENSION_PIN_V &&
        pinV <= Config::MAX_ENTRADA_ADC_V && adc < 4095;
    grupoValido = grupoValido && muestraValida;
    if (!muestraValida) resultado.valida = false;
    sumaMiliboltios += milivoltios;
    sumaAdc += adc;
    if (++contador < Config::MUESTRAS_TENSION) return resultado;

    resultado.voltajePinV = sumaMiliboltios /
        (1000.0f * Config::MUESTRAS_TENSION);
    resultado.adc = resultado.adc_crudo = sumaAdc / Config::MUESTRAS_TENSION;
    resultado.ultimaLecturaMs = ahoraMs;
    resultado.lista = true;
    resultado.valida = grupoValido;
    resultado.voltaje = resultado.voltajePinV;
    resultado.voltajeBateria = resultado.porcentaje = 0.0f;
    if (grupoValido) {
        float entradaV = resultado.voltajePinV;
        if (Config::ESCALONAR_TENSION)
            entradaV = roundf(entradaV / Config::PASO_TENSION_PIN_V) *
                       Config::PASO_TENSION_PIN_V;
        entradaV = constrain(entradaV, Config::ENTRADA_TENSION_MIN_V,
                            Config::ENTRADA_TENSION_MAX_V);
        resultado.voltaje = entradaV;
        resultado.voltajeBateria = tensionPackDesdeVoltajePin(entradaV);
        resultado.porcentaje = 100.0f * (entradaV - Config::ENTRADA_TENSION_MIN_V) /
            (Config::ENTRADA_TENSION_MAX_V - Config::ENTRADA_TENSION_MIN_V);
    }
    contador = 0;
    sumaMiliboltios = sumaAdc = 0;
    grupoValido = true;
    return resultado;
}
