#include "Compartidos.h"
#include "Rtos.h"
#include <string.h>

namespace {
DatosCompartidos datos;
SemaphoreHandle_t mtxDatos = nullptr;
volatile bool solicitudCalibrar = false;

template <typename T>
void escribir(T& destino, const T& origen) {
    if (xSemaphoreTake(mtxDatos, portMAX_DELAY) == pdTRUE) {
        destino = origen;
        xSemaphoreGive(mtxDatos);
    }
}

template <typename T>
T copiar(const T& origen) {
    T r;
    memset(&r, 0, sizeof(r));
    if (xSemaphoreTake(mtxDatos, portMAX_DELAY) == pdTRUE) {
        r = origen;
        xSemaphoreGive(mtxDatos);
    }
    return r;
}
} // namespace

bool inicializarCompartidos() {
    memset(&datos, 0, sizeof(datos));
    mtxDatos = xSemaphoreCreateMutex();
    return mtxDatos != nullptr;
}

void publicarCorriente(const MedidaCorriente& m)  { escribir(datos.corriente, m); }
void publicarTension(const MedidaTension& m)      { escribir(datos.tension, m); }
void publicarEnergia(const EstadoEnergia& e)      { escribir(datos.energia, e); }
void publicarVelocidad(const DatosVelocidad& v)   { escribir(datos.velocidad, v); }
void publicarGPS(const DatosGPS& g)               { escribir(datos.gps, g); }
void publicarTemperatura(const DatosTemperatura& t) { escribir(datos.temperatura, t); }
void publicarPrimerPulso(bool recibido)           { escribir(datos.primerPulsoRecibido, recibido); }

MedidaCorriente leerCorriente()   { return copiar(datos.corriente); }
MedidaTension leerTension()       { return copiar(datos.tension); }
EstadoEnergia leerEnergia()       { return copiar(datos.energia); }
DatosVelocidad leerVelocidad()    { return copiar(datos.velocidad); }
DatosGPS leerGPS()                { return copiar(datos.gps); }
DatosTemperatura leerTemperatura() { return copiar(datos.temperatura); }
bool leerPrimerPulso()            { return copiar(datos.primerPulsoRecibido); }

void solicitarCalibracion() { solicitudCalibrar = true; }
bool consumirSolicitudCalibracion() {
    if (!solicitudCalibrar) return false;
    solicitudCalibrar = false;
    return true;
}
