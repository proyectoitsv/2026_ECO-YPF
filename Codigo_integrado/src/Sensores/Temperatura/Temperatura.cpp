#include "Temperatura.h"

// Inicializamos OneWire y DallasTemperature con el pin asignado
GestorTemperatura::GestorTemperatura(uint8_t pin) : oneWire(pin), sensores(&oneWire) {}

void GestorTemperatura::inicializar() {
    sensores.begin();
    // La conversión se consulta después desde main, sin detener el loop().
    sensores.setWaitForConversion(false);
}

void GestorTemperatura::solicitarTemperaturas() {
    sensores.requestTemperatures();
}

float GestorTemperatura::leerTemperatura(uint8_t indice) {
    return sensores.getTempCByIndex(indice);
}
