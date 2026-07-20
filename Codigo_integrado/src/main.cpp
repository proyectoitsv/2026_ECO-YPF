#include <Arduino.h>
#include "Sensores/Tension/Tension.h"
#include "Sensores/Temperatura/Temperatura.h" // Asegúrate de tener esta cabecera

// Objetos globales
GestorTemperatura sensorTemp(15); // GPIO 15 para temperatura

void setup() {
    Serial.begin(115200);
    pinMode(34, INPUT); // Tu configuración original de batería
    sensorTemp.inicializar();
}

void loop() {
    // 1. Lectura de Batería (Tu código original)
    MedidaTension datos = leerTensionCompleta(); 

    Serial.print("Voltaje: "); Serial.print(datos.voltaje, 3);
    Serial.print(" V | ADC: "); Serial.print(datos.adc);
    Serial.print(" | Porcentaje: "); Serial.print(datos.porcentaje, 2);
    Serial.print(" % | ");

    // 2. Lectura de Temperatura (Nueva integración)
    sensorTemp.solicitarTemperaturas();
    float temp = sensorTemp.leerTemperatura(0);
    
    Serial.print("Temperatura: ");
    Serial.print(temp);
    Serial.println(" °C");

    delay(100);
}