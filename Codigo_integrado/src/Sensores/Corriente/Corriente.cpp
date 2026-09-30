#include "Corriente.h"
#include <Arduino.h>
#include <math.h>

namespace {
enum class Fase { MEDICION, ESTABILIZANDO, CALIBRANDO };
Fase fase = Fase::MEDICION;
MedidaCorriente resultado = {};
float muestrasV[Config::MUESTRAS_CORRIENTE] = {};
uint8_t contador = 0, ciclosReposo = 0;
uint32_t ultimoMuestreoMs = 0, inicioEstabilizacionMs = 0;
uint32_t muestrasCalibracion = 0;
double sumaCalibracionV = 0.0;
bool grupoValido = true, configuracionValida = false, ajusteEnReposo = false;

bool entradaValida(float pinV, uint16_t adc) {
    return pinV >= Config::MIN_CORRIENTE_PIN_V &&
           pinV <= Config::MAX_ENTRADA_ADC_V && adc < 4095;
}

float mediaRecortada() {
    for (uint8_t i = 0; i + 1 < Config::MUESTRAS_CORRIENTE; ++i)
        for (uint8_t j = 0; j + 1 < Config::MUESTRAS_CORRIENTE - i; ++j)
            if (muestrasV[j] > muestrasV[j + 1]) {
                const float temporal = muestrasV[j];
                muestrasV[j] = muestrasV[j + 1];
                muestrasV[j + 1] = temporal;
            }
    float suma = 0.0f;
    for (uint8_t i = Config::DESCARTAR_EXTREMOS_CORRIENTE;
         i < Config::MUESTRAS_CORRIENTE - Config::DESCARTAR_EXTREMOS_CORRIENTE; ++i)
        suma += muestrasV[i];
    return suma / (Config::MUESTRAS_CORRIENTE -
                  2 * Config::DESCARTAR_EXTREMOS_CORRIENTE);
}
}

void recalibrarSensorCorriente() {
    if (!configuracionValida) return;
    fase = Fase::ESTABILIZANDO;
    inicioEstabilizacionMs = millis();
    muestrasCalibracion = 0;
    sumaCalibracionV = 0.0;
    contador = ciclosReposo = 0;
    grupoValido = true;
    resultado.lista = resultado.valida = false;
    resultado.calibrando = true;
    resultado.corrienteA = 0.0f;
}

void inicializarSensorCorriente(bool calibrarOffset) {
    configuracionValida = Config::errorConfiguracion() == nullptr;
    resultado = {};
    resultado.offsetSensorV = Config::OFFSET_CORRIENTE_V;
    contador = ciclosReposo = 0;
    grupoValido = true;
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

    const float pinV = analogReadMilliVolts(Config::PIN_CORRIENTE) / 1000.0f;
    const uint16_t adc = analogRead(Config::PIN_CORRIENTE);
    const bool valida = entradaValida(pinV, adc);
    if (fase == Fase::CALIBRANDO) {
        resultado.voltajePinV = pinV;
        resultado.voltajeSensorV = pinV * Config::FACTOR_SALIDA_SENSOR_SOBRE_ADC;
        if (!valida) {
            // Nunca calibrar un cero con una entrada desconectada/saturada.
            muestrasCalibracion = 0;
            sumaCalibracionV = 0.0;
            return false;
        }
        sumaCalibracionV += resultado.voltajeSensorV;
        if (++muestrasCalibracion >= Config::MUESTRAS_CALIBRACION) {
            resultado.offsetSensorV = sumaCalibracionV / muestrasCalibracion;
            resultado.calibrando = false;
            fase = Fase::MEDICION;
            contador = 0;
            grupoValido = true;
        }
        return false;
    }

    muestrasV[contador++] = pinV;
    grupoValido = grupoValido && valida;
    // La integración se suspende al detectar una muestra inválida.
    if (!valida) resultado.valida = false;
    if (contador < Config::MUESTRAS_CORRIENTE) return false;

    resultado.voltajePinV = mediaRecortada();
    resultado.voltajeSensorV = resultado.voltajePinV *
                              Config::FACTOR_SALIDA_SENSOR_SOBRE_ADC;
    resultado.ultimaLecturaMs = ahoraMs;
    resultado.lista = true;
    resultado.valida = grupoValido;
    resultado.corrienteA = grupoValido ?
        corrienteDesdeVoltajeSensor(resultado.voltajeSensorV,
                                    resultado.offsetSensorV) : 0.0f;
    if (grupoValido && ajusteEnReposo &&
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
    grupoValido = true;
    return true;
}

MedidaCorriente obtenerMedidaCorriente() { return resultado; }
bool sensorCorrienteListo() { return resultado.lista && resultado.valida &&
                                    !resultado.calibrando; }
float obtenerCorrienteA() { return resultado.corrienteA; }
float obtenerSalidaSensorV() { return resultado.voltajeSensorV; }
float obtenerOffsetSensorV() { return resultado.offsetSensorV; }
uint32_t obtenerPromedioADCmV() {
    return static_cast<uint32_t>(roundf(resultado.voltajePinV * 1000.0f));
}
