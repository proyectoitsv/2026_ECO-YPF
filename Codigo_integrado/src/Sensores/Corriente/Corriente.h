#ifndef SENSOR_CORRIENTE_H
#define SENSOR_CORRIENTE_H

#include <Arduino.h>

// Pin del sensor
extern const int PIN_SENSOR;

// Modo de alimentación del sensor (true = 5V, false = 3.3V)
extern const bool ALIMENTACION_5V;

// Configura el pin y hace la calibración inicial de offset. Llamar en setup().
void inicializarSensorCorriente();

// Llamar en cada vuelta del loop(). Devuelve true cuando hay una lectura
// nueva disponible (una vez cada N_MUESTRAS llamadas).
bool actualizarSensorCorriente();

// Última corriente calculada, en Amperes.
float obtenerCorriente();

// Último valor de ADC promediado (media recortada).
int obtenerPromedioADC();

// Offset de reposo actual (se ajusta dinámicamente).
float obtenerOffset();

// Fuerza una recalibración manual del offset (bloqueante).
void recalibrarSensorCorriente();

#endif