#include "Tension.h"

// --- PARÁMETROS CONFIGURABLES ---
const float VOLTAJE_MIN = 0.675;  // 0% de batería (Voltios en el pin para 48V)
const float VOLTAJE_MAX = 2.055;  // 100% de batería (Voltios en el pin para 56V)
const float BATT_MIN    = 48.0;   // Tensión real mínima de la batería
const float BATT_MAX    = 56.0;   // Tensión real máxima de la batería
const int   PIN_ANALOG  = 34;     // Pin analógico de lectura
// --------------------------------

MedidaTension leerTensionCompleta() {
    MedidaTension resultado;

    long suma = 0;
    for (int i = 0; i < 30; i++) {
        suma += analogRead(PIN_ANALOG);
        delay(2);
    }
    //resultado.adc = (int)(suma / 30);
    resultado.voltaje = analogReadMilliVolts(PIN_ANALOG) / 1000.0;

    // Lectura del voltaje real en el pin del ESP32 (con referencia de 3.3V)
    //resultado.voltaje = (resultado.adc * 3.3 / 4095.0);

    // Limitar el voltaje leído dentro del rango configurado
    float voltajeLimpio = constrain(resultado.voltaje, VOLTAJE_MIN, VOLTAJE_MAX);
    
    // Mapeo matemático directo mediante proporción (regla de tres / interpolación lineal)
    float proporcion = (voltajeLimpio - VOLTAJE_MIN) / (VOLTAJE_MAX - VOLTAJE_MIN);

    // Cálculo del porcentaje de la batería (0% a 100%)
    resultado.porcentaje = proporcion * 100.0;
    resultado.porcentaje = constrain(resultado.porcentaje, 0.0, 100.0);

    // Cálculo directo de la tensión real de la batería basada en los puntos extremos dados
    resultado.voltajeBateria = proporcion * (BATT_MAX - BATT_MIN) + BATT_MIN;

    resultado.adc_crudo = analogRead(PIN_ANALOG);
    
    return resultado;
}