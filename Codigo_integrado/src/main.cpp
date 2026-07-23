#include <Arduino.h>
#include "Sensores/Tension/Tension.h"
#include "Sensores/Temperatura/Temperatura.h"

// Objetos globales
GestorTemperatura sensorTemp(15); // GPIO 15 para temperatura

void setup() {
    Serial.begin(115200);
    pinMode(34, INPUT); // Pin analógico de la batería
    sensorTemp.inicializar();
}

void loop() {
    // 1. Lectura de Batería 
    MedidaTension datos = leerTensionCompleta(); 

    Serial.print("Voltaje: "); Serial.print(datos.voltaje, 3);
    Serial.print(" V | ADC: "); Serial.print(datos.adc);
    Serial.print(" | Porcentaje: "); Serial.print(datos.porcentaje, 2);
    Serial.print(" % | Tensión Batería: "); Serial.print(datos.voltajeBateria, 2);
    Serial.print(" V | ");

    // 2. Lectura de Temperatura
    sensorTemp.solicitarTemperaturas();
    float temp = sensorTemp.leerTemperatura(0);
    
    Serial.print("Temperatura: ");
    Serial.print(temp);
    Serial.println(" °C");

    delay(100);
}