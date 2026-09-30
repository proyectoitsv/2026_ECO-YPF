#ifndef SENSOR_CORRIENTE_H
#define SENSOR_CORRIENTE_H

#include <stdint.h>

// ACS758-050B bidireccional alimentado a 5 V: sensibilidad nominal.
static const float SENSIBILIDAD_ACS758_V_POR_A = 0.040f;

// Conversión física independiente del ADC. Con offset nominal de 2,500 V,
// una salida simulada de 2,840 V equivale a 8,5 A.
inline float corrienteDesdeVoltajeSensor(float salidaSensorV,
                                        float offsetSensorV = 2.500f) {
    return (salidaSensorV - offsetSensorV) / SENSIBILIDAD_ACS758_V_POR_A;
}

extern const int PIN_SENSOR_CORRIENTE;
extern const float FACTOR_SALIDA_SENSOR_SOBRE_ADC;

// En la prueba de banco con 2,840 V en el pin, usar calibrarOffset=false.
// Con el ACS758 real, usar true y arrancar sin corriente para medir el cero.
void inicializarSensorCorriente(bool calibrarOffset);

// Se llama frecuentemente desde loop(); devuelve true cada 15 muestras nuevas.
bool actualizarSensorCorriente();
bool sensorCorrienteListo();

float obtenerCorrienteA();
float obtenerSalidaSensorV();
float obtenerOffsetSensorV();
uint32_t obtenerPromedioADCmV();

#endif
