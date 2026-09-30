#ifndef CONFIGURACION_ECO_YPF_H
#define CONFIGURACION_ECO_YPF_H

#include <stdint.h>
#include <math.h>

// Editar aquí y volver a compilar/cargar. Los módulos comparten estos valores.
namespace Config {
enum class ModoEnergia {
    FUENTES_ANALOGICAS, SENSORES_REALES, REFERENCIA_SIMULADA,
    VOLTAJE_ADC_INYECTADO, ACS_TENSION_FIJA
};
constexpr ModoEnergia MODO = ModoEnergia::FUENTES_ANALOGICAS;

// Tensión: dos puntos del circuito existente, no un divisor supuesto.
constexpr uint8_t PIN_TENSION = 34;
constexpr float ENTRADA_TENSION_MIN_V = 0.450f;
constexpr float ENTRADA_TENSION_MAX_V = 1.250f;
constexpr float PACK_MIN_V = 48.0f;
constexpr float PACK_MAX_V = 52.0f;
constexpr uint32_t INTERVALO_TENSION_MS = 50;
constexpr uint8_t MUESTRAS_TENSION = 5;
constexpr float TOLERANCIA_TENSION_PIN_V = 0.030f;
constexpr bool ESCALONAR_TENSION = false;
constexpr float PASO_TENSION_PIN_V = 0.025f; // Solo si ESCALONAR_TENSION=true.

// Corriente: ACS758-050B nominal, alimentación de 5 V.
constexpr uint8_t PIN_CORRIENTE = 35;
constexpr float OFFSET_CORRIENTE_V = 2.500f;
constexpr float SENSIBILIDAD_CORRIENTE_V_POR_A = 0.040f;
// V_salida_sensor / V_entrada_GPIO35. Fuente directa en banco: 1,0.
// Este factor NO protege eléctricamente la entrada del ESP32.
constexpr float FACTOR_SALIDA_SENSOR_SOBRE_ADC = 1.0f;
constexpr float SIGNO_CORRIENTE = 1.0f; // -1 si la descarga invierte la salida.
constexpr uint32_t INTERVALO_CORRIENTE_MS = 5;
constexpr uint8_t MUESTRAS_CORRIENTE = 15;
constexpr uint8_t DESCARTAR_EXTREMOS_CORRIENTE = 2; // En cada extremo.

enum class CalibracionCorriente { OFFSET_FIJO, AUTOMATICA_EN_CERO };
// Solo para ACS real. Los modos con fuentes SIEMPRE usan OFFSET_CORRIENTE_V.
constexpr CalibracionCorriente CALIBRACION_CORRIENTE =
    CalibracionCorriente::AUTOMATICA_EN_CERO;
constexpr uint32_t ESTABILIZACION_CORRIENTE_MS = 500;
constexpr uint16_t MUESTRAS_CALIBRACION = 500;
constexpr uint32_t INTERVALO_CALIBRACION_MS = 2;
// Opcional: una carga pequeña puede confundirse con reposo. Banco: siempre off.
constexpr bool AJUSTAR_OFFSET_EN_REPOSO = false;
constexpr float UMBRAL_REPOSO_A = 0.15f;
constexpr uint8_t CICLOS_REPOSO = 5;
constexpr float ALPHA_OFFSET = 0.05f;

// ADC y disponibilidad. El máximo es un límite de aceptación, no protección.
constexpr uint8_t RESOLUCION_ADC_BITS = 12;
constexpr float MIN_CORRIENTE_PIN_V = 0.100f;
constexpr float MAX_ENTRADA_ADC_V = 3.100f;
constexpr uint32_t MAX_ANTIGUEDAD_LECTURA_MS = 1500;

// Cuatro baterías EN SERIE: el pack mantiene 17 Ah.
constexpr float CAPACIDAD_PACK_AH = 17.0f;
constexpr float TENSION_NOMINAL_PACK_V = 48.0f;
constexpr float TENSION_REFERENCIA_V = TENSION_NOMINAL_PACK_V;
constexpr float CORRIENTE_REFERENCIA_A = 8.5f;

constexpr uint32_t BAUDIOS_MONITOR = 115200;
constexpr uint32_t INTERVALO_REPORTE_MS = 1000;
// Código GPS/temperatura/inductivo disponible; banco usa solo tensión/corriente.
constexpr bool HABILITAR_SENSORES_AUXILIARES = false;

// Un error detiene la adquisición y muestra su motivo en el monitor.
inline const char* errorConfiguracion() {
    const bool pinTensionValido = (PIN_TENSION >= 32 && PIN_TENSION <= 36) ||
                                 PIN_TENSION == 39;
    const bool pinCorrienteValido = (PIN_CORRIENTE >= 32 && PIN_CORRIENTE <= 36) ||
                                   PIN_CORRIENTE == 39;
    if (!pinTensionValido || !pinCorrienteValido || PIN_TENSION == PIN_CORRIENTE ||
        (HABILITAR_SENSORES_AUXILIARES && (PIN_TENSION == 33 || PIN_CORRIENTE == 33)))
        return "Revisar pines ADC1 disponibles y conflictos con temperatura GPIO33.";
    if (!isfinite(MAX_ENTRADA_ADC_V) || MAX_ENTRADA_ADC_V <= 0.0f ||
        MAX_ENTRADA_ADC_V > 3.1f || RESOLUCION_ADC_BITS != 12)
        return "Revisar limite ADC (hasta 3.1 V) y resolucion (12 bits).";
    if (!isfinite(ENTRADA_TENSION_MIN_V) || !isfinite(ENTRADA_TENSION_MAX_V) ||
        ENTRADA_TENSION_MIN_V <= 0.0f ||
        ENTRADA_TENSION_MAX_V <= ENTRADA_TENSION_MIN_V ||
        ENTRADA_TENSION_MAX_V > MAX_ENTRADA_ADC_V ||
        !isfinite(PACK_MIN_V) || !isfinite(PACK_MAX_V) ||
        PACK_MIN_V <= 0.0f || PACK_MAX_V <= PACK_MIN_V)
        return "Rango de tension invalido.";
    if (!isfinite(OFFSET_CORRIENTE_V) || OFFSET_CORRIENTE_V <= 0.0f ||
        !isfinite(SENSIBILIDAD_CORRIENTE_V_POR_A) ||
        SENSIBILIDAD_CORRIENTE_V_POR_A <= 0.0f ||
        !isfinite(FACTOR_SALIDA_SENSOR_SOBRE_ADC) ||
        FACTOR_SALIDA_SENSOR_SOBRE_ADC <= 0.0f ||
        (SIGNO_CORRIENTE != 1.0f && SIGNO_CORRIENTE != -1.0f) ||
        !isfinite(MIN_CORRIENTE_PIN_V) || MIN_CORRIENTE_PIN_V <= 0.0f ||
        MIN_CORRIENTE_PIN_V >= MAX_ENTRADA_ADC_V ||
        OFFSET_CORRIENTE_V / FACTOR_SALIDA_SENSOR_SOBRE_ADC < MIN_CORRIENTE_PIN_V ||
        OFFSET_CORRIENTE_V / FACTOR_SALIDA_SENSOR_SOBRE_ADC >= MAX_ENTRADA_ADC_V)
        return "Revisar offset, sensibilidad, factor y signo de corriente.";
    if (!isfinite(CAPACIDAD_PACK_AH) || CAPACIDAD_PACK_AH <= 0.0f ||
        !isfinite(TENSION_NOMINAL_PACK_V) || TENSION_NOMINAL_PACK_V <= 0.0f ||
        !isfinite(TENSION_REFERENCIA_V) || TENSION_REFERENCIA_V <= 0.0f ||
        !isfinite(CORRIENTE_REFERENCIA_A))
        return "Capacidad/tension nominal o referencia invalidas.";
    if (MUESTRAS_TENSION == 0 || MUESTRAS_CORRIENTE == 0 ||
        2 * DESCARTAR_EXTREMOS_CORRIENTE >= MUESTRAS_CORRIENTE ||
        MUESTRAS_CALIBRACION == 0 || INTERVALO_TENSION_MS == 0 ||
        INTERVALO_CORRIENTE_MS == 0 || INTERVALO_CALIBRACION_MS == 0 ||
        INTERVALO_REPORTE_MS == 0 || MAX_ANTIGUEDAD_LECTURA_MS == 0 ||
        static_cast<uint32_t>(MUESTRAS_TENSION) * INTERVALO_TENSION_MS >=
            MAX_ANTIGUEDAD_LECTURA_MS ||
        static_cast<uint32_t>(MUESTRAS_CORRIENTE) * INTERVALO_CORRIENTE_MS >=
            MAX_ANTIGUEDAD_LECTURA_MS)
        return "Muestreo/filtro invalido o tiempo de lectura demasiado corto.";
    if (!isfinite(TOLERANCIA_TENSION_PIN_V) || TOLERANCIA_TENSION_PIN_V < 0.0f ||
        !isfinite(PASO_TENSION_PIN_V) || PASO_TENSION_PIN_V <= 0.0f ||
        !isfinite(UMBRAL_REPOSO_A) || UMBRAL_REPOSO_A < 0.0f ||
        CICLOS_REPOSO == 0 || !isfinite(ALPHA_OFFSET) ||
        ALPHA_OFFSET <= 0.0f || ALPHA_OFFSET > 1.0f)
        return "Tolerancia, escalonado o ajuste de offset invalidos.";
    return nullptr;
}
} // namespace Config
#endif
