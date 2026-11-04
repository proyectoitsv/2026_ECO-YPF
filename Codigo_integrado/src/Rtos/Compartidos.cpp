#include "Compartidos.h"
#include "Rtos.h"
#include <string.h>
#include <esp_timer.h>
#include <algorithm>
#include "../Registro/ConfigRegistro.h"
#include "../Tareas/ModoEnergia.h"

namespace {
// Una sola copia de los últimos valores publicados. Las tareas no comparten
// punteros a sensores: toman o dejan una copia breve protegida por mtxDatos.
DatosCompartidos datos;
SemaphoreHandle_t mtxDatos = nullptr;
volatile bool solicitudCalibrar = false;
AcumuladorElectrico acumulador;

template <typename T>
void escribir(T& destino, const T& origen) {
    // El candado dura solo la copia; nunca se usa para esperar al GPS o a la SD.
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
    GestorEnergia inicial(Config::CAPACIDAD_PACK_AH,Config::TENSION_NOMINAL_PACK_V);
    datos.energia = datos.infoEnergia.estado = inicial.obtenerEstado();
    // Antes de la primera medición el modelo muestra su capacidad nominal;
    // la calidad SIN_DATO del CSV permite distinguirla de una medida real.
    datos.infoEnergia.modo=uint8_t(Config::MODO);
    datos.infoEnergia.tensionADC=usaTensionADC(); datos.infoEnergia.corrienteADC=usaCorrienteADC();
    return mtxDatos != nullptr;
}

void publicarCorriente(const MedidaCorriente& m) {
    xSemaphoreTake(mtxDatos,portMAX_DELAY);
    if (m.lista && (!datos.corrienteUs || m.ultimaLecturaMs != datos.corriente.ultimaLecturaMs))
        datos.corrienteUs = esp_timer_get_time() - uint64_t(uint32_t(millis()-m.ultimaLecturaMs))*1000;
    datos.corriente=m; xSemaphoreGive(mtxDatos);
}
void publicarTension(const MedidaTension& m) {
    xSemaphoreTake(mtxDatos,portMAX_DELAY);
    if (m.lista && (!datos.tensionUs || m.ultimaLecturaMs != datos.tension.ultimaLecturaMs))
        datos.tensionUs = esp_timer_get_time() - uint64_t(uint32_t(millis()-m.ultimaLecturaMs))*1000;
    datos.tension=m; xSemaphoreGive(mtxDatos);
}
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

void publicarInfoEnergia(const InfoEnergiaRegistro& e) {
    xSemaphoreTake(mtxDatos,portMAX_DELAY);
    datos.energia=e.estado; datos.infoEnergia=e;
    // El resumen deja de dar cobertura cuando una entrada lleva demasiado
    // tiempo sin actualizarse; no inventa muestras en ese tramo.
    const uint64_t hasta=std::min(e.tensionUs,e.corrienteUs)+uint64_t(RegistroConfig::FRESCO_ADC_MS)*1000;
    // Un snapshot pudo cerrar la ventana mientras esta publicacion esperaba el mutex.
    // No retroceder el cursor del resumen ni contar dos veces ese intervalo.
    actualizarResumen(acumulador,std::max(e.capturaUs,acumulador.ultimo),e.estado.corrienteA,e.estado.potenciaW,e.disponible,hasta);
    xSemaphoreGive(mtxDatos);
}
SnapshotRegistro capturarSnapshotRegistro() {
    SnapshotRegistro s;
    xSemaphoreTake(mtxDatos,portMAX_DELAY);
    // Foto de valores que llegaron en distintos momentos, con sus edades.
    // El resumen se cierra en esta misma foto y empieza otra ventana.
    s.capturaUs=esp_timer_get_time(); s.datos=datos;
    s.resumen=cerrarResumen(acumulador,s.capturaUs);
    xSemaphoreGive(mtxDatos); return s;
}
DatosCompartidos copiarDatosCompartidos() { return copiar(datos); }

void solicitarCalibracion() { solicitudCalibrar = true; }
bool consumirSolicitudCalibracion() {
    if (!solicitudCalibrar) return false;
    solicitudCalibrar = false;
    return true;
}
