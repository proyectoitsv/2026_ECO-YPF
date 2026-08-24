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
unsigned long tiempoActual = 0;
bool pulsoNuevo = false;
volatile bool primerPulso = true;
volatile bool estatico = false;
volatile unsigned long tiempoInicio = 0;
volatile unsigned long tiempoGuardado = 0;

const byte TAMANO_VECTOR = 10;
float lecturasVelocidad[TAMANO_VECTOR] = {0.0};
byte indiceLectura = 0;
byte totalLecturas = 0;

void IRAM_ATTR cuentaPulsos() {
    tiempoActual = millis();
    if (primerPulso) {
        tiempoInicio = tiempoActual;
        primerPulso = false;
    } 
    else {
        tiempoGuardado = tiempoActual - tiempoInicio;
        tiempoInicio = tiempoActual;
        pulsoNuevo = true;
    }
}




// Objetos globales de temperatura
GestorTemperatura sensorTemp1(25); // Primer sensor
GestorTemperatura sensorTemp2(33); // Segundo sensor


bool estadoPin = false;
void setup() {
    Serial.begin(115200);
    

    // Configuración Batería
    pinMode(34, INPUT); // Pin analógico de la batería
    pinMode(12, OUTPUT);
    
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
/*
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
    
    /*Serial.print("Temp1: "); Serial.print(temp1);
    Serial.print(" °C | Temp2: "); Serial.print(temp2);
    Serial.print(" °C | ");

*/
    // 3. Lectura de Velocidad
    if (pulsoNuevo) {
        digitalWrite(12, !digitalRead(12));
        pulsoNuevo = false;

        noInterrupts();
        unsigned long tiempoCopia = tiempoGuardado;
        pulsoNuevo = false;
        interrupts();
        Serial.print("Tiempo entre pulsos: "); Serial.print(tiempoCopia); Serial.println(" ms");
        // Calcular velocidad en km/h
        if (tiempoCopia > 15) {
            velocidadKmH = ((1539.38 * 14 / 80) / tiempoCopia) * 3.6; // (2*pi*r 14/80)/T
            Serial.print("Velocidad Actual (");
            Serial.print(velocidadKmH, 2);
            Serial.println(" km/h)");
            //vector para almacenar las últimas 10 lecturas de velocidad
            lecturasVelocidad[indiceLectura] = velocidadKmH;
            indiceLectura = (indiceLectura + 1) % TAMANO_VECTOR;

        if (totalLecturas < TAMANO_VECTOR) 
            {
                totalLecturas++;
            }

        if (totalLecturas == TAMANO_VECTOR/2)
            {
            Serial.print("Velocidad Actual (");
            Serial.print(velocidadKmH, 2);
            Serial.println(" km/h)");
            }
        if (totalLecturas == TAMANO_VECTOR) 
            {
            // Cálculo del promedio con un bucle FOR
            totalLecturas = 0;
            float suma = 0.0;
            for (byte i = 0; i < TAMANO_VECTOR; i++) {
                suma += lecturasVelocidad[i];
                }
            float promedioVelocidad = suma / TAMANO_VECTOR;

            Serial.print("Velocidad Promedio (");

            Serial.print(promedioVelocidad, 2);
            Serial.println(" km/h)");
            }   

        }
    }
if ((millis() - tiempoActual > 7000) & primerPulso == false) 
    {
    primerPulso = true;
    Serial.println("Velocidad Actual (0.00 km/h)");
    totalLecturas = 0; // Reinicia las muestras del promedio
    indiceLectura = 0;
    }  
    
}
        