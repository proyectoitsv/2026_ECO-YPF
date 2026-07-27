#include <Arduino.h>
#include "Sensores/Tension/Tension.h"
#include "Sensores/Temperatura/Temperatura.h"

// Objetos globales
GestorTemperatura sensorTemp1(25); // Primer sensor
GestorTemperatura sensorTemp2(33); // Segundo sensor

void setup() {
    Serial.begin(115200);
    pinMode(34, INPUT); // Pin analógico de la batería
    
    // Inicialización de ambos sensores
    sensorTemp1.inicializar();
    sensorTemp2.inicializar();
}

void loop() {
    // 1. Lectura de Batería (INTACTO)
    MedidaTension datos = leerTensionCompleta(); 

    Serial.print("Voltaje: "); Serial.print(datos.voltaje, 3);
    Serial.print(" V | ADC: "); Serial.print(datos.adc);
    Serial.print(" | Porcentaje: "); Serial.print(datos.porcentaje, 2);
    Serial.print(" % | Tensión Batería: "); Serial.print(datos.voltajeBateria, 2);
    Serial.print(" V | ");

    // 2. Lectura de Temperatura y Promedio
    sensorTemp1.solicitarTemperaturas();
    sensorTemp2.solicitarTemperaturas();
    
    float temp1 = sensorTemp1.leerTemperatura(0);
    float temp2 = sensorTemp2.leerTemperatura(0);
    
    // Cálculo del promedio
    float promedio = (temp1 + temp2) / 2.0;
    
    Serial.print("Temp1: "); Serial.print(temp1);
    Serial.print(" °C | Temp2: "); Serial.print(temp2);
    Serial.print(" °C | Promedio: ");
    Serial.print(promedio);
    Serial.println(" °C");

    delay(100);
}