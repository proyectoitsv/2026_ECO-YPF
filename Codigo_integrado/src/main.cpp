#include <Arduino.h>
#include "Sensores/Tension/Tension.h"

void setup() {
    Serial.begin(115200);
    pinMode(34, INPUT);
}

void loop() {
    MedidaTension datos = leerTensionCompleta(); 

    Serial.print("Voltaje: ");
    Serial.print(datos.voltaje, 3);

    Serial.print(" V | ADC: ");
    Serial.print(datos.adc);
    Serial.print(" | ");

    Serial.print("Porcentaje: ");
    Serial.print(datos.porcentaje, 2);
    Serial.print(" %");
    Serial.print(" | ");

    Serial.print("Voltaje de Batería: ");
    Serial.print(datos.voltajeBateria, 2);
    Serial.print("V");
    Serial.print(" | ");

    Serial.print("ADC Crudo: ");
    Serial.println(datos.adc_crudo);

    delay(100);
}