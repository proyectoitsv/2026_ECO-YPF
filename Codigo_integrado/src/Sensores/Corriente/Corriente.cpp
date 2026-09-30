#include "Corriente.h"

#include <Arduino.h>
#include <math.h>

// GPIO35 pertenece a ADC1 y está libre en el proyecto integrado.
// GPIO34 ya está reservado para el módulo de tensión.
const int PIN_SENSOR_CORRIENTE = 35;

// V_salida_ACS758 / V_entrada_GPIO35. Ajustar al divisor/adaptador REAL.
// 1,0 solo corresponde a una conexión sin divisor; no protege de una salida
// del ACS758 que supere el límite eléctrico del ESP32.
const float FACTOR_SALIDA_SENSOR_SOBRE_ADC = 1.0f;

namespace {
const uint8_t N_MUESTRAS = 15;
const uint8_t N_DESCARTAR = 2;
const uint32_t TIEMPO_MUESTRA_MS = 5;
const uint16_t MUESTRAS_CAL_INICIAL = 500;
const uint8_t RETARDO_CAL_MS = 2;
const float UMBRAL_REPOSO_A = 0.15f;
const uint8_t CICLOS_REPOSO_REQ = 5;
const float ALPHA_AJUSTE = 0.05f;

uint32_t muestrasADCmV[N_MUESTRAS] = {};
uint8_t contadorMuestras = 0;
uint32_t ultimoMuestreoMs = 0;
uint32_t promedioADCmV = 0;
float offsetSensorV = 2.500f;
float salidaSensorV = 0.0f;
float corrienteA = 0.0f;
uint8_t ciclosEnReposo = 0;
bool lecturaLista = false;
bool autoajustarOffset = false;

uint32_t mediaRecortada(uint32_t valores[], uint8_t cantidad) {
    // Ordenar 15 muestras cada 75 ms evita que picos aislados dominen la media.
    for (uint8_t i = 0; i < cantidad - 1; ++i) {
        for (uint8_t j = 0; j < cantidad - i - 1; ++j) {
            if (valores[j] > valores[j + 1]) {
                const uint32_t temporal = valores[j];
                valores[j] = valores[j + 1];
                valores[j + 1] = temporal;
            }
        }
    }

    uint32_t suma = 0;
    for (uint8_t i = N_DESCARTAR; i < cantidad - N_DESCARTAR; ++i) {
        suma += valores[i];
    }
    return suma / (cantidad - 2 * N_DESCARTAR);
}
}  // namespace

void inicializarSensorCorriente(bool calibrarOffset) {
    pinMode(PIN_SENSOR_CORRIENTE, INPUT);
    analogSetPinAttenuation(PIN_SENSOR_CORRIENTE, ADC_11db);

    offsetSensorV = 2.500f;
    autoajustarOffset = calibrarOffset;

    if (calibrarOffset) {
        // Solo en setup(): durante esta calibración no debe circular corriente.
        uint32_t sumaMiliboltios = 0;
        for (uint16_t i = 0; i < MUESTRAS_CAL_INICIAL; ++i) {
            sumaMiliboltios += analogReadMilliVolts(PIN_SENSOR_CORRIENTE);
            delay(RETARDO_CAL_MS);
        }
        const float promedioAdcV =
            (sumaMiliboltios / static_cast<float>(MUESTRAS_CAL_INICIAL)) / 1000.0f;
        offsetSensorV = promedioAdcV * FACTOR_SALIDA_SENSOR_SOBRE_ADC;
    }

    contadorMuestras = 0;
    ciclosEnReposo = 0;
    lecturaLista = false;
    corrienteA = 0.0f;
    ultimoMuestreoMs = millis() - TIEMPO_MUESTRA_MS;
}

bool actualizarSensorCorriente() {
    const uint32_t ahoraMs = millis();
    if (ahoraMs - ultimoMuestreoMs < TIEMPO_MUESTRA_MS) return false;
    ultimoMuestreoMs = ahoraMs;

    muestrasADCmV[contadorMuestras++] =
        analogReadMilliVolts(PIN_SENSOR_CORRIENTE);
    if (contadorMuestras < N_MUESTRAS) return false;

    promedioADCmV = mediaRecortada(muestrasADCmV, N_MUESTRAS);
    salidaSensorV = (promedioADCmV / 1000.0f) *
                    FACTOR_SALIDA_SENSOR_SOBRE_ADC;
    corrienteA = corrienteDesdeVoltajeSensor(salidaSensorV, offsetSensorV);

    // Se conserva el ajuste lento de reposo de la rama original. En el modo
    // de voltaje inyectado no se ajusta el offset nominal de 2,500 V.
    if (autoajustarOffset && fabsf(corrienteA) < UMBRAL_REPOSO_A) {
        if (++ciclosEnReposo >= CICLOS_REPOSO_REQ) {
            offsetSensorV += ALPHA_AJUSTE * (salidaSensorV - offsetSensorV);
        }
        corrienteA = 0.0f;
    } else {
        ciclosEnReposo = 0;
    }

    contadorMuestras = 0;
    lecturaLista = true;
    return true;
}

bool sensorCorrienteListo() { return lecturaLista; }
float obtenerCorrienteA() { return corrienteA; }
float obtenerSalidaSensorV() { return salidaSensorV; }
float obtenerOffsetSensorV() { return offsetSensorV; }
uint32_t obtenerPromedioADCmV() { return promedioADCmV; }
