#ifndef TEMPERATURA_H
#define TEMPERATURA_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

class GestorTemperatura {
public:
    GestorTemperatura(uint8_t pin);
    void inicializar();
    void solicitarTemperaturas();
    float leerTemperatura(uint8_t indice);
    bool alimentacionParasitaria() const { return parasitaria; }

private:
    OneWire oneWire;
    DallasTemperature sensores;
    bool parasitaria = false;
};

#endif
