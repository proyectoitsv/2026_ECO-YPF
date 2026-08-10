#include <Arduino.h>
#include "Sensores/Tension/Tension.h"
#include "Sensores/Temperatura/Temperatura.h"
#include "Sensores/Inductivo(velocidad)/Inductivo.h"
// --- CONFIGURACIÓN SENSOR INDUCTIVO ---
const int pinSensorInductivo = 26;
const int pulsosPorRevolucion = 1;
const float diametroRuedaMetros = 0.49;



float tiempoResta=0;



float circunferencia = 3.141592 * diametroRuedaMetros;
float rpm = 0.0;
float velocidadKmH = 0.0;

// ISR para el sensor inductivo

volatile unsigned long contadorPulsos = 0;
unsigned long pulsosCopiados = 0;
volatile bool tiempoIniciado = false; //sincroniza la base de tiempo con el primer pulso del sensor inductivo
void IRAM_ATTR cuentaPulsos() {
    if (tiempoIniciado == true)
    {
        contadorPulsos++;
    }
    else
    {
        tiempoIniciado = true;
        contadorPulsos = 1; // Inicia el contador de pulsos
        timerWrite(timer, 0); //reinicio timer
        timerAlarmEnable(timer); //habilito timer
    }
}

//ISR para el temporizador
hw_timer_t *timer = NULL;
volatile bool temporizadorListo = false;

void IRAM_ATTR onTimer() {
    timerAlarmDisable(timer);
    temporizadorListo = true; 
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

    // Configuración del temporizador
    timer = timerBegin(0, 80, true); //un tick cada 1 microsegundo
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 1000000, true); //alarma cada 1.5 segundos
    timerAlarmEnable(timer);
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
    // 3. Temporizador 
    
    if (temporizadorListo) { 
        noInterrupts(); // Deshabilita interrupciones para evitar conflict
        
        pulsosCopiados = contadorPulsos;
        contadorPulsos = 0;
        temporizadorListo = false; // Reinicia la bandera
        tiempoIniciado = false; // Reinicia la bandera
        interrupts(); 
        velocidadKmH = (pulsosCopiados * circunferencia * 3.6) /1.5; // Convertir RPM a km/h
    }
    
    //Serial.println("RPM: "); Serial.print(rpm, 1);
    Serial.print(" | Velocidad: "); Serial.print(velocidadKmH, 2);
    Serial.print(" km/h");
    
        Serial.print(" | Vueltas: "); Serial.println(pulsosCopiados); 


    delay(100);
}