#ifndef COMPARTIDOS_H
#define COMPARTIDOS_H

#include <Arduino.h>
#include "../Sensores/Corriente/Corriente.h"
#include "../Sensores/Tension/Tension.h"
#include "../Sensores/Energia/Energia.h"
#include "../Sensores/GPS/GPS.h"
#include "../Registro/Resumen.h"

struct DatosVelocidad {
    float rpm;
    float kmh;
    uint32_t pulsos;       // Pulsos de la última ventana de 1 s.
    uint64_t totalPulsos;
    uint32_t muestra, perdidas;
    uint64_t capturaUs;
};

struct DatosTemperatura {
    float temp1;
    float temp2;
    float promedio;
    bool valida;
    bool valida1, valida2, visto1, visto2;
    bool parasitaria1, parasitaria2;
    uint64_t captura1Us, captura2Us;
};

struct InfoEnergiaRegistro {
    EstadoEnergia estado;
    MedidaTension tensionUsada;
    MedidaCorriente corrienteUsada;
    double tiempoIntegradoS;
    uint64_t capturaUs, tensionUs, corrienteUs;
    uint32_t segmento;
    uint8_t modo;
    bool activa, disponible, tensionADC, corrienteADC;
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
    uint64_t tensionUs, corrienteUs;
    InfoEnergiaRegistro infoEnergia;
};

struct SnapshotRegistro {
    DatosCompartidos datos;
    ResumenElectrico resumen;
    uint64_t capturaUs;
};
void publicarInfoEnergia(const InfoEnergiaRegistro&);
SnapshotRegistro capturarSnapshotRegistro();
DatosCompartidos copiarDatosCompartidos();

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
