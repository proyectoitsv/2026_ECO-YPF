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
float velocidadKmH = 0.0;

// ISR para el sensor inductivo
int tiempoActual = 0;
int tiempoInicio = 0;
int tiempoGuardado = 0;
void IRAM_ATTR cuentaPulsos() {
    millis();
    tiempoActual = millis();
    tiempoGuardado = tiempoActual - tiempoInicio;
    velocidadKmH = 1539.38 / tiempoGuardado; // (2*pi*r)/T
    Serial.print(" | Velocidad: "); Serial.print(velocidadKmH, 2);
    Serial.println(" km/h");
    tiempoInicio = millis();
}



// Objetos globales de temperatura
GestorTemperatura sensorTemp1(25); // Primer sensor
GestorTemperatura sensorTemp2(33); // Segundo sensor


bool estadoPin = false;
void setup() {
    Serial.begin(115200);
    

    // Configuración Batería
    pinMode(34, INPUT); // Pin analógico de la batería
    
    
    // Configuración Temperatura
    sensorTemp1.inicializar();
    sensorTemp2.inicializar();

    // Configuración Inductivo (Velocidad)
    pinMode(pinSensorInductivo, INPUT);
    attachInterrupt(digitalPinToInterrupt(pinSensorInductivo), cuentaPulsos, RISING);


    // --- GENERADOR DE FRECUENCIA DE PRUEBA 
    int frecuenciaHz = 15;         
    ledcSetup(0, frecuenciaHz, 8); // Configura el canal 0 
    ledcAttachPin(27, 0);          // Asigna el canal 0 al Pin 27
    ledcWrite(0, 127);             //el ciclo de trabajo deberia ser el 50% porq si no, no mide bien
}

void loop() {
    // 1. Lectura de Batería
    MedidaTension datos = leerTensionCompleta(); 
/*
    Serial.print("Voltaje: "); Serial.print(datos.voltaje, 3);
    Serial.print(" V | ADC: "); Serial.print(datos.adc);
    Serial.print(" | Tensión Batería: "); Serial.print(datos.voltajeBateria, 2);
    Serial.print(" V | ");
*/
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
    delay(100);
}