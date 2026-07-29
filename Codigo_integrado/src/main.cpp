#include <Arduino.h>
#include "Sensores/Tension/Tension.h"
#include "Sensores/Temperatura/Temperatura.h"
#include "Sensores/Inductivo(velocidad)/Inductivo.h"
// --- CONFIGURACIÓN SENSOR INDUCTIVO ---
const int pinSensorInductivo = 26;
const int pulsosPorRevolucion = 1;
const float diametroRuedaMetros = 0.49;


float tiempoSegundos1 = 0.0f; // Variable global para almacenar el tiempo transcurrido en segundos

volatile unsigned long contadorPulsos = 0;
unsigned long tiempoAnteriorInductivo = 0;
const unsigned long intervaloInductivo = 1000; // Recálculo cada 1 segundo
unsigned long pulsosCopiados = 0;
float tiempoResta=0;

float circunferencia = 3.141592 * diametroRuedaMetros;
float rpm = 0.0;
float velocidadKmH = 0.0;

// ISR para el sensor inductivo
void IRAM_ATTR cuentaPulsos() {
    contadorPulsos++;
}


//ISR para el temporizador
hw_timer_t *timer = NULL;
volatile bool temporizadorListo = false;

void IRAM_ATTR onTimer() {
    temporizadorListo = true; 
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

    // Configuración del temporizador
    timer = timerBegin(0, 80, true); //un tick cada 1 microsegundo
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarm(timer, 1000000, true); //alarma cada 1 segundo

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


    // 3. Temporizador 
    if (temporizadorListo) { //adentro de este if pongan todo, yo puse lo que decia abajo para q guarde la variable pero siga contando
        temporizadorListo = false; // Reinicia la bandera
        unsigned long pulsosCopiados = contadorPulsos;
        contadorPulsos = 0;
        velocidadKmH = pulsosCopiados * circunferencia * 3.6; // Convertir RPM a km/h
    }

    /* 4. Lectura de Inductivo (Velocidad / RPM)
    unsigned long tiempoActual = millis();
    if (tiempoActual - tiempoAnteriorInductivo >= intervaloInductivo) {
        
        noInterrupts();
        tiempoResta = tiempoActual - tiempoAnteriorInductivo;
        
        //unsigned long pulsosCopiados = contadorPulsos;
        pulsosCopiados = contadorPulsos;
        contadorPulsos = 0;
        interrupts();

        rpm = (pulsosCopiados * 60.0 * 1000) / (pulsosPorRevolucion * tiempoResta);
        float circunferencia = 3.141592 * diametroRuedaMetros;
        float rps = rpm / 60.0;
        velocidadKmH = rps * circunferencia * 3.6;
        

        tiempoAnteriorInductivo = tiempoActual;
        */
    }
   
    Serial.println("Tiempo Resta"); Serial.print(tiempoResta, 3);
    //Serial.println("RPM: "); Serial.print(rpm, 1);
    Serial.print(" | Velocidad: "); Serial.print(velocidadKmH, 2);
    Serial.print(" km/h");
    
    Serial.print(pulsosCopiados); 
    Serial.println(" s");

    delay(100);
}