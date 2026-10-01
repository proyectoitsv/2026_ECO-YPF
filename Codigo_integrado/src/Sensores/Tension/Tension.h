#ifndef TENSION_H
#define TENSION_H
#include <stdint.h>
#include "../../Configuracion.h"

struct MedidaTension {
    // Campos anteriores conservados para compatibilidad.
    float voltaje;          // Entrada utilizada para convertir.
    int adc;
    float porcentaje;      // Indicador por tensión; NO es el SOC energético.
    float voltajeBateria;  // Pack equivalente en volts.
    int adc_crudo;
    float voltajePinV;     // Promedio simple de todas las muestras.
    uint32_t ultimaLecturaMs;
    bool lista;            // Ya existe al menos un promedio completo.
};

// Dos puntos de calibración del circuito existente. La conversión es lineal;
// fuera de esos puntos se extrapola, sin recortar ni rechazar la entrada.
inline float tensionPackDesdeVoltajePin(float voltajePinV) {
    return Config::PACK_MIN_V +
        (voltajePinV - Config::ENTRADA_TENSION_MIN_V) *
        (Config::PACK_MAX_V - Config::PACK_MIN_V) /
        (Config::ENTRADA_TENSION_MAX_V - Config::ENTRADA_TENSION_MIN_V);
}
void inicializarTension();
MedidaTension leerTensionCompleta(); // No espera; conserva la última lectura.
#endif
