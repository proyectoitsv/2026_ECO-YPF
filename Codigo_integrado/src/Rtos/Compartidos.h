#ifndef COMPARTIDOS_H
#define COMPARTIDOS_H

#include <Arduino.h>
#include "../Sensores/Corriente/Corriente.h"
#include "../Sensores/Tension/Tension.h"
#include "../Sensores/Energia/Energia.h"
#include "../Sensores/GPS/GPS.h"

struct DatosVelocidad {
    float rpm;
    float kmh;
    uint32_t pulsos;       // Pulsos de la última ventana de 1 s.
    uint64_t totalPulsos;
};

struct DatosTemperatura {
    float temp1;
    float temp2;
    float promedio;
    bool valida;
};

// Snapshots entre tareas. Cada campo lo escribe UNA sola tarea; el resto lo copia.
struct DatosCompartidos {
    MedidaCorriente corriente;
    MedidaTension tension;
    EstadoEnergia energia;
    DatosVelocidad velocidad;
    DatosGPS gps;
    DatosTemperatura temperatura;
    bool primerPulsoRecibido;
};

// Crea el mutex y deja todo en cero. Llamar antes de crear las tareas.
bool inicializarCompartidos();

void publicarCorriente(const MedidaCorriente& m);
void publicarTension(const MedidaTension& m);
void publicarEnergia(const EstadoEnergia& e);
void publicarVelocidad(const DatosVelocidad& v);
void publicarGPS(const DatosGPS& g);
void publicarTemperatura(const DatosTemperatura& t);
void publicarPrimerPulso(bool recibido);

MedidaCorriente leerCorriente();
MedidaTension leerTension();
EstadoEnergia leerEnergia();
DatosVelocidad leerVelocidad();
DatosGPS leerGPS();
DatosTemperatura leerTemperatura();
bool leerPrimerPulso();

// Bandera para recalibrar el offset de corriente: la pide cualquier tarea
// (comando Serial) y la consume la tarea dueña de Corriente.
void solicitarCalibracion();
bool consumirSolicitudCalibracion();

#endif
