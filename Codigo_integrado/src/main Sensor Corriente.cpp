//------------------------------------------------------------------------------------------------------------------
//EL CODIGO ESTE FUE PROBADO EN UNA ESP32C3, POR LO TANTO SE DEBEN DE CAMBIAR LOS PINES Y LA CONFIGURACION DEL .INI
//------------------------------------------------------------------------------------------------------------------
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "Corriente/Corriente.h"
void setup() {
Serial.begin(9600);

  Serial.println("Calibrando offset de reposo del ACS758...");
  inicializarSensorCorriente();

  Serial.print("Offset inicial: ");
  Serial.println(obtenerOffset(), 2);
  Serial.println("Enviar 'c' por Serial para forzar una recalibracion manual.");
}

void loop() {
      if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'c' || c == 'C') {
      Serial.println("Recalibrando offset manualmente...");
      recalibrarSensorCorriente();
      Serial.print("Nuevo offset: ");
      Serial.println(obtenerOffset(), 2);
    }
  }

  if (actualizarSensorCorriente()) {
    Serial.print("ADC Promedio: ");
    Serial.print(obtenerPromedioADC());
    Serial.print(" | Offset: ");
    Serial.print(obtenerOffset(), 2);
    Serial.print(" | Corriente: ");
    Serial.print(obtenerCorriente(), 2);
    Serial.println(" A");
  }

  delay(100);
}