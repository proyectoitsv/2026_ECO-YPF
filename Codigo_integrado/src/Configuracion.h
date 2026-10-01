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
// Se promedian todas las muestras y se convierte sin limitar ni escalonar.

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
// Promedio simple: ninguna muestra se descarta por su valor.

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

// Resolución ADC. No hay rechazo de lecturas por rango o saturación.
constexpr uint8_t RESOLUCION_ADC_BITS = 12;

// Cuatro baterías EN SERIE: el pack mantiene 17 Ah.
constexpr float CAPACIDAD_PACK_AH = 17.0f;
constexpr float TENSION_NOMINAL_PACK_V = 48.0f;
constexpr float TENSION_REFERENCIA_V = TENSION_NOMINAL_PACK_V;
constexpr float CORRIENTE_REFERENCIA_A = 8.5f;

constexpr uint32_t BAUDIOS_MONITOR = 115200;
constexpr uint32_t INTERVALO_REPORTE_MS = 1000;
// Código GPS/temperatura/inductivo disponible; banco usa solo tensión/corriente.
constexpr bool HABILITAR_SENSORES_AUXILIARES = false;

// Verifica los parámetros al arrancar, no los valores medidos por el ADC.
// Evita divisiones por cero y configuraciones de pines/muestreo incoherentes.
inline const char* errorConfiguracion() {
    const bool pinTensionValido = (PIN_TENSION >= 32 && PIN_TENSION <= 36) ||
                                 PIN_TENSION == 39;
    const bool pinCorrienteValido = (PIN_CORRIENTE >= 32 && PIN_CORRIENTE <= 36) ||
                                   PIN_CORRIENTE == 39;
    if (!pinTensionValido || !pinCorrienteValido || PIN_TENSION == PIN_CORRIENTE ||
        (HABILITAR_SENSORES_AUXILIARES && (PIN_TENSION == 33 || PIN_CORRIENTE == 33)))
        return "Revisar pines ADC1 disponibles y conflictos con temperatura GPIO33.";
    if (RESOLUCION_ADC_BITS != 12)
        return "Revisar resolucion ADC (12 bits).";
    if (!isfinite(ENTRADA_TENSION_MIN_V) || !isfinite(ENTRADA_TENSION_MAX_V) ||
        ENTRADA_TENSION_MIN_V <= 0.0f ||
        ENTRADA_TENSION_MAX_V <= ENTRADA_TENSION_MIN_V ||
        !isfinite(PACK_MIN_V) || !isfinite(PACK_MAX_V) ||
        PACK_MIN_V <= 0.0f || PACK_MAX_V <= PACK_MIN_V)
        return "Rango de tension invalido.";
    if (!isfinite(OFFSET_CORRIENTE_V) || OFFSET_CORRIENTE_V <= 0.0f ||
        !isfinite(SENSIBILIDAD_CORRIENTE_V_POR_A) ||
        SENSIBILIDAD_CORRIENTE_V_POR_A <= 0.0f ||
        !isfinite(FACTOR_SALIDA_SENSOR_SOBRE_ADC) ||
        FACTOR_SALIDA_SENSOR_SOBRE_ADC <= 0.0f ||
        (SIGNO_CORRIENTE != 1.0f && SIGNO_CORRIENTE != -1.0f))
        return "Revisar offset, sensibilidad, factor y signo de corriente.";
    if (!isfinite(CAPACIDAD_PACK_AH) || CAPACIDAD_PACK_AH <= 0.0f ||
        !isfinite(TENSION_NOMINAL_PACK_V) || TENSION_NOMINAL_PACK_V <= 0.0f ||
        !isfinite(TENSION_REFERENCIA_V) || TENSION_REFERENCIA_V <= 0.0f ||
        !isfinite(CORRIENTE_REFERENCIA_A))
        return "Capacidad/tension nominal o referencia invalidas.";
    if (MUESTRAS_TENSION == 0 || MUESTRAS_CORRIENTE == 0 ||
        MUESTRAS_CALIBRACION == 0 || INTERVALO_TENSION_MS == 0 ||
        INTERVALO_CORRIENTE_MS == 0 || INTERVALO_CALIBRACION_MS == 0 ||
        INTERVALO_REPORTE_MS == 0)
        return "Cantidad de muestras o intervalo invalido.";
    if (!isfinite(UMBRAL_REPOSO_A) || UMBRAL_REPOSO_A < 0.0f ||
        CICLOS_REPOSO == 0 || !isfinite(ALPHA_OFFSET) ||
        ALPHA_OFFSET <= 0.0f || ALPHA_OFFSET > 1.0f)
        return "Parametros de ajuste de offset invalidos.";
    return nullptr;
}
} // namespace Config
#endif
