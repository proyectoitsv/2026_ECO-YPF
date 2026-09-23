#ifndef GPS_H
#define GPS_H

#include <Arduino.h>

// Estructura con los datos "planos" del GPS, lista para usar desde main.cpp
struct DatosGPS {
    bool     ubicacionValida;
    double   latitud;
    double   longitud;
    float    velocidadKmH;
    float    rumboGrados;
    bool     rumboValido;   // false = vehículo detenido, sin rumbo confiable
    float    hdop;
    uint32_t satelites;
    uint16_t anio;
    uint8_t  mes;
    uint8_t  dia;
    uint8_t  hora;
    uint8_t  minuto;
    uint8_t  segundo;
};

// Configura Serial2, espera al GPS y envía el comando de ahorro de energía
void inicializarGPS();

// Debe llamarse en cada loop(). Lee lo disponible en Serial2 (no bloquea).
void actualizarGPS();

// Devuelve true UNA vez cuando hay una posición nueva (se resetea al leerla)
bool datosGPSActualizados();

// Devuelve la última lectura completa del GPS
DatosGPS obtenerDatosGPS();

#endif