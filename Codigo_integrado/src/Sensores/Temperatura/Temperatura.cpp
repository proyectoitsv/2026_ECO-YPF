#include "Temperatura.h"

// Inicializamos OneWire y DallasTemperature con el pin asignado
GestorTemperatura::GestorTemperatura(uint8_t pin) : oneWire(pin), sensores(&oneWire) {}

void GestorTemperatura::inicializar() {
    sensores.begin();
}

void solicitarTemperaturas() {
    // Esta función se puede llamar antes de leer para pedir la conversión
}

void GestorTemperatura::solicitarTemperaturas() {
    sensores.requestTemperatures();
}

float GestorTemperatura::leerTemperatura(uint8_t indice) {
    return sensores.getTempCByIndex(indice);
}