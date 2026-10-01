#ifndef SENSOR_CORRIENTE_H
#define SENSOR_CORRIENTE_H
#include <stdint.h>
#include "../../Configuracion.h"

struct MedidaCorriente {
    float voltajePinV;
    float voltajeSensorV;  // Reconstruido con el factor del acondicionamiento.
    float offsetSensorV;  // Cero efectivo; puede diferir del configurado.
    float corrienteA;
    uint32_t ultimaLecturaMs;
    bool lista;           // Ya existe al menos un promedio completo.
    bool calibrando;
};

inline float corrienteDesdeVoltajeSensor(float salidaV, float offsetV) {
    return Config::SIGNO_CORRIENTE * (salidaV - offsetV) /
           Config::SENSIBILIDAD_CORRIENTE_V_POR_A;
}
void inicializarSensorCorriente(bool calibrarOffset);
bool actualizarSensorCorriente(); // Muestreo y calibración SIN delay().
void recalibrarSensorCorriente(); // Solo llamar sin carga en un ACS real.
MedidaCorriente obtenerMedidaCorriente();

bool sensorCorrienteListo();
float obtenerCorrienteA();
float obtenerSalidaSensorV();
float obtenerOffsetSensorV();
uint32_t obtenerPromedioADCmV();
#endif
