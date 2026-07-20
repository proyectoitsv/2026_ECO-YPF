#include "Temperatura.h"

GestorTemperatura::GestorTemperatura(uint8_t pin) : oneWire(pin), sensores(&oneWire) {}

void GestorTemperatura::inicializar() {
    sensores.begin();
}

void GestorTemperatura::solicitarTemperaturas() {
    sensores.requestTemperatures();
}

float GestorTemperatura::leerTemperatura(uint8_t indice) {
    return sensores.getTempCByIndex(indice);
}