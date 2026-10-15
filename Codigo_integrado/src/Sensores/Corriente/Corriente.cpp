#include "Corriente.h"
#include <Arduino.h>
#include <math.h>

namespace {
enum class Fase { MEDICION, ESTABILIZANDO, CALIBRANDO };
Fase fase = Fase::MEDICION;
MedidaCorriente resultado = {};
uint32_t sumaMiliboltios = 0;
uint8_t contador = 0, ciclosReposo = 0;
uint32_t ultimoMuestreoMs = 0, inicioEstabilizacionMs = 0;
uint32_t muestrasCalibracion = 0;
double sumaCalibracionV = 0.0;
bool configuracionValida = false, ajusteEnReposo = false;
}

void recalibrarSensorCorriente() {
    if (!configuracionValida) return;
    fase = Fase::ESTABILIZANDO;
    inicioEstabilizacionMs = millis();
    muestrasCalibracion = 0;
    sumaCalibracionV = 0.0;
    contador = ciclosReposo = 0;
    sumaMiliboltios = 0;
    resultado.lista = false;
    resultado.calibrando = true;
    resultado.corrienteA = 0.0f;
}

void inicializarSensorCorriente(bool calibrarOffset) {
    configuracionValida = Config::errorConfiguracion() == nullptr;
    resultado = {};
    resultado.offsetSensorV = Config::OFFSET_CORRIENTE_V;
    contador = ciclosReposo = 0;
    sumaMiliboltios = 0;
    fase = Fase::MEDICION;
    ajusteEnReposo = calibrarOffset && Config::AJUSTAR_OFFSET_EN_REPOSO;
    ultimoMuestreoMs = millis() - Config::INTERVALO_CORRIENTE_MS;
    if (!configuracionValida) return;
    pinMode(Config::PIN_CORRIENTE, INPUT);
    analogReadResolution(Config::RESOLUCION_ADC_BITS);
    analogSetPinAttenuation(Config::PIN_CORRIENTE, ADC_11db);
    if (calibrarOffset) recalibrarSensorCorriente();
}

bool actualizarSensorCorriente() {
    if (!configuracionValida) return false;
    const uint32_t ahoraMs = millis();
    if (fase == Fase::ESTABILIZANDO) {
        if (ahoraMs - inicioEstabilizacionMs < Config::ESTABILIZACION_CORRIENTE_MS)
            return false;
        fase = Fase::CALIBRANDO;
        ultimoMuestreoMs = ahoraMs - Config::INTERVALO_CALIBRACION_MS;
    }
    const uint32_t intervalo = fase == Fase::CALIBRANDO ?
        Config::INTERVALO_CALIBRACION_MS : Config::INTERVALO_CORRIENTE_MS;
    if (ahoraMs - ultimoMuestreoMs < intervalo) return false;
    ultimoMuestreoMs = ahoraMs;

    // Una única conversión por muestra. No se descarta por rango/ADC crudo.
    const uint32_t milivoltios = analogReadMilliVolts(Config::PIN_CORRIENTE);
    if (fase == Fase::CALIBRANDO) {
        resultado.voltajePinV = milivoltios / 1000.0f;
        resultado.voltajeSensorV = resultado.voltajePinV *
                                  Config::FACTOR_SALIDA_SENSOR_SOBRE_ADC;
        sumaCalibracionV += resultado.voltajeSensorV;
        if (++muestrasCalibracion >= Config::MUESTRAS_CALIBRACION) {
            resultado.offsetSensorV = sumaCalibracionV / muestrasCalibracion;
            resultado.calibrando = false;
            fase = Fase::MEDICION;
            contador = 0;
            sumaMiliboltios = 0;
        }
        return false;
    }

    sumaMiliboltios += milivoltios;
    if (++contador < Config::MUESTRAS_CORRIENTE) return false;

    // Promedio simple: cada muestra aporta al resultado, sin ordenarlas.
    resultado.voltajePinV = sumaMiliboltios /
        (1000.0f * Config::MUESTRAS_CORRIENTE);
    resultado.voltajeSensorV = resultado.voltajePinV *
                              Config::FACTOR_SALIDA_SENSOR_SOBRE_ADC;
    resultado.ultimaLecturaMs = ahoraMs;
    resultado.lista = true;
    resultado.corrienteA = corrienteDesdeVoltajeSensor(resultado.voltajeSensorV,
                                                     resultado.offsetSensorV);
    if (ajusteEnReposo &&
        fabsf(resultado.corrienteA) < Config::UMBRAL_REPOSO_A) {
        if (ciclosReposo < Config::CICLOS_REPOSO) ++ciclosReposo;
        if (ciclosReposo >= Config::CICLOS_REPOSO)
            resultado.offsetSensorV += Config::ALPHA_OFFSET *
                (resultado.voltajeSensorV - resultado.offsetSensorV);
        resultado.corrienteA = 0.0f;
    } else {
        ciclosReposo = 0;
    }
    contador = 0;
    sumaMiliboltios = 0;
    return true;
}

MedidaCorriente obtenerMedidaCorriente() { return resultado; }
bool sensorCorrienteListo() { return resultado.lista && !resultado.calibrando; }
float obtenerCorrienteA() { return resultado.corrienteA; }
float obtenerSalidaSensorV() { return resultado.voltajeSensorV; }
float obtenerOffsetSensorV() { return resultado.offsetSensorV; }
uint32_t obtenerPromedioADCmV() {
    return static_cast<uint32_t>(roundf(resultado.voltajePinV * 1000.0f));
}
