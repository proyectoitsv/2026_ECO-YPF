#include <Arduino.h>
#include "Sensores/Tension/Tension.h"
#include "Sensores/Temperatura/Temperatura.h"
#include "Sensores/Inductivo(velocidad)/Inductivo.h"
// --- CONFIGURACIÓN SENSOR INDUCTIVO ---
const int pinSensorInductivo = 26;
const int pulsosPorRevolucion = 1;
const float diametroRuedaMetros = 0.49;



float tiempoResta=0;



float circunferencia = PI * diametroRuedaMetros;
float rpm = 0.0;
float velocidadKmH = 0.0;

// ISR para el sensor inductivo
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED; // Protege las variables compartidas
volatile bool contando = false; 
volatile bool temporizadorListo = false;
u_int64_t tiempo = 0;


hw_timer_t *timer = NULL;
volatile unsigned long contadorPulsos = 0;
volatile unsigned long pulsosCopiados = 0;
volatile bool tiempoIniciado = false; //sincroniza la base de tiempo con el primer pulso del sensor inductivo
void IRAM_ATTR cuentaPulsos() {

    if (!contando && !temporizadorListo) {
        contando = true;
        contadorPulsos = 1; 
        
        
        timerWrite(timer, 0);       
        timerAlarmEnable(timer);  
        timerStart(timer);          
    } 

    else if (contando) {
        portENTER_CRITICAL_ISR(&mux);
        contadorPulsos++;
        portEXIT_CRITICAL_ISR(&mux);
    
    }
}

//ISR para el temporizador




void IRAM_ATTR onTimer() {
    
    timerStop(timer);             
    timerAlarmDisable(timer);      
    digitalWrite(12, !digitalRead(12)); 
    contando = false;              
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
    pinMode(12, OUTPUT);  
    
    
    // Configuración Temperatura
    sensorTemp1.inicializar();
    sensorTemp2.inicializar();

    // Configuración Inductivo (Velocidad)
    pinMode(pinSensorInductivo, INPUT);
    attachInterrupt(digitalPinToInterrupt(pinSensorInductivo), cuentaPulsos, RISING);

    // Configuración del temporizador
    timer = timerBegin(0, 80, true); //un tick cada 1 microsegundo
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 1000000, false); //alarma cada 1 segundos
    timerStop(timer);         
    timerAlarmDisable(timer);
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
       
        
        pulsosCopiados = contadorPulsos;
        contadorPulsos = 0;

        Serial.print(" | pulsos: "); Serial.println(pulsosCopiados); 

            velocidadKmH = ((pulsosCopiados / 4) * circunferencia * 3.6) * 14 / 20; // Convertir RPM a km/h

        Serial.print(" | Velocidad: "); Serial.print(velocidadKmH, 2);
        Serial.print(" km/h");   
        Serial.print(" | pulsos1: "); Serial.println(pulsosCopiados); 
        Serial.print(" | tiempo: "); Serial.println((u_int32_t)tiempo);


         
        temporizadorListo = false; // Reinicia la bandera
    }

   


}