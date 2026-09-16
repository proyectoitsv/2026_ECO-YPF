#include <Arduino.h>
#include "Sensores/Tension/Tension.h"
#include "Sensores/Temperatura/Temperatura.h"
#include "Sensores/Inductivo(velocidad)/Inductivo.h"
// --- CONFIGURACIÓN SENSOR INDUCTIVO ---
const int pinSensorInductivo = 26;
const int pulsosPorRevolucion = 1;
const float diametroRuedaMetros = 0.49;


float tiempoSegundos1 = 0.0f; // Variable global para almacenar el tiempo transcurrido en segundos
float circunferencia = 3.141592 * diametroRuedaMetros;
float rpm = 0.0;
float VelocidadMs = 0.0;
float velocidadKmH = 0.0;

// ISR para el sensor inductivo
int tiempoActual = 0;
int tiempoInicio = 0;
int tiempoGuardado = 0;
bool calculo = 0;
void IRAM_ATTR cuentaPulsos() {
    tiempoActual = millis();
    tiempoGuardado = tiempoActual - tiempoInicio;
    calculo = 1;
    tiempoInicio = millis();
}



// Objetos globales de temperatura
GestorTemperatura sensorTemp1(25); // Primer sensor
GestorTemperatura sensorTemp2(33); // Segundo sensor


bool estadoPin = false;
void setup() {
    Serial.begin(115200);
    millis();

    // Configuración Batería
    pinMode(34, INPUT); // Pin analógico de la batería
    
    
    // Configuración Temperatura
    sensorTemp1.inicializar();
    sensorTemp2.inicializar();

    // Configuración Inductivo (Velocidad)
    pinMode(pinSensorInductivo, INPUT);
    attachInterrupt(digitalPinToInterrupt(pinSensorInductivo), cuentaPulsos, RISING);

}

void loop() {
    // 1. Lectura de Batería
    MedidaTension datos = leerTensionCompleta(); 
    

    Serial.print("Voltaje: "); Serial.print(datos.voltaje, 3);
    Serial.print(" V | ADC: "); Serial.print(datos.adc_crudo);
    Serial.print(" | Tensión Batería: "); Serial.print(datos.voltajeBateria, 2);
    Serial.print(" V | ");
    Serial.print(datos.porcentaje);
    Serial.println(" % | ");

    // 2. Lectura de Temperatura y Promedio
    sensorTemp1.solicitarTemperaturas();
    sensorTemp2.solicitarTemperaturas();
    
    float temp1 = sensorTemp1.leerTemperatura(0);
    float temp2 = sensorTemp2.leerTemperatura(0);
    float promedio = (temp1 + temp2) / 2.0;
    
    /*Serial.print("Temp1: "); Serial.print(temp1);
    Serial.print(" °C | Temp2: "); Serial.print(temp2);
    Serial.print(" °C | ");

*/ 

   /* if (calculo)
        {
        if (tiempoGuardado > 30)
            {
            velocidadMs = 1539.38 / tiempoGuardado; // (2*pi*r)/T
            velocidadKmH = velocidadMs * 3600 / 1000
            Serial.print(" | Velocidad: "); Serial.print(velocidadKmH, 1);
            Serial.print(" km/h");
            Serial.print(" | Tiempo: "); Serial.println(tiempoGuardado, 2)
            }
        calculo = 0;
        }
    delay(100); */
}