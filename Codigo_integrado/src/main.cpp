#include <Arduino.h>
#include "Sensores/Tension/Tension.h"
#include "Sensores/Temperatura/Temperatura.h"

// --- CONFIGURACIÓN SENSOR INDUCTIVO ---
const int pinSensorInductivo = 26;
const int pulsosPorRevolucion = 5;
const float diametroRuedaMetros = 0.20;

volatile unsigned long contadorPulsos = 0;
unsigned long tiempoAnteriorInductivo = 0;
const unsigned long intervaloInductivo = 1000; // Recálculo cada 1 segundo

float rpm = 0.0;
float velocidadKmH = 0.0;

// ISR para el sensor inductivo
void IRAM_ATTR cuentaPulsos() {
    contadorPulsos++;
}

// Objetos globales de temperatura
GestorTemperatura sensorTemp1(25); // Primer sensor
GestorTemperatura sensorTemp2(33); // Segundo sensor

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
}

void loop() {
    // 1. Lectura de Batería
    MedidaTension datos = leerTensionCompleta(); 

    Serial.print("Voltaje: "); Serial.print(datos.voltaje, 3);
    Serial.print(" V | ADC: "); Serial.print(datos.adc);
    Serial.print(" | Tensión Batería: "); Serial.print(datos.voltajeBateria, 2);
    Serial.print(" V | ");

    // 2. Lectura de Temperatura y Promedio
    sensorTemp1.solicitarTemperaturas();
    sensorTemp2.solicitarTemperaturas();
    
    float temp1 = sensorTemp1.leerTemperatura(0);
    float temp2 = sensorTemp2.leerTemperatura(0);
    float promedio = (temp1 + temp2) / 2.0;
    
    Serial.print("Temp1: "); Serial.print(temp1);
    Serial.print(" °C | Temp2: "); Serial.print(temp2);
    Serial.print(" °C | ");

    // 3. Lectura de Inductivo (Velocidad / RPM)
    unsigned long tiempoActual = millis();
    if (tiempoActual - tiempoAnteriorInductivo >= intervaloInductivo) {
        noInterrupts();
        unsigned long pulsosCopiados = contadorPulsos;
        contadorPulsos = 0;
        interrupts();

        rpm = (pulsosCopiados * 60.0) / pulsosPorRevolucion;
        float circunferencia = 3.141592 * diametroRuedaMetros;
        float rps = rpm / 60.0;
        velocidadKmH = rps * circunferencia * 3.6;

        tiempoAnteriorInductivo = tiempoActual;
    }

    Serial.println("RPM: "); Serial.print(rpm, 1);
    Serial.print(" | Velocidad: "); Serial.print(velocidadKmH, 2);
    Serial.println(" km/h");

    delay(100);
}