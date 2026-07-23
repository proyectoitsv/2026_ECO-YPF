#include "Tension.h"

// --- PARÁMETROS CONFIGURABLES ---
const float VOLTAJE_MIN     = 0.576;   // 0% de batería (Voltios en pin)
const float VOLTAJE_MAX     = 1.970;   // 100% de batería (Voltios en pin)
const float BATT_MIN        = 48.0;    // Tensión real mínima de la batería (V)
const float BATT_MAX        = 56.0;    // Tensión real máxima de la batería (V)
const int   PIN_ANALOG      = 34;      // Pin analógico de lectura

// --- PARÁMETROS DE TIEMPO Y FILTRADO ---
const unsigned long TIEMPO_MUESTRA  = 50;     // Muestra cada 50 ms
const unsigned long TIEMPO_REPORTE  = 2000;   // Actualización cada 2 segundos (2000 ms)
const int           CANT_MUESTRAS   = 10;     // Promedio de 10 muestras
const float         PASO_MINIMO_V   = 0.100;  // Umbral de cambio de 100 mV (0.1 V)
// --------------------------------

MedidaTension leerTensionCompleta() {
    // Variables estáticas para mantener el estado entre llamadas consecutivas en el loop()
    static MedidaTension resultado = {0};
    static float ultimoVoltajeReportado = -1.0;

    static unsigned long ultimoMuestreo = 0;
    static unsigned long ultimoReporte  = 0;

    static long acumMiliboltios = 0;
    static int  contadorMuestras = 0;
    static float voltajePromedioTemp = 0.0;

    unsigned long tiempoActual = millis();

    // 1. Muestreo no bloqueante (Cada 50 ms)
    if (tiempoActual - ultimoMuestreo >= TIEMPO_MUESTRA) {
        ultimoMuestreo = tiempoActual;

        acumMiliboltios += analogReadMilliVolts(PIN_ANALOG);
        contadorMuestras++;

        // Al acumular 10 muestras, calculamos el promedio puntual
        if (contadorMuestras >= CANT_MUESTRAS) {
            voltajePromedioTemp = (acumMiliboltios / (float)CANT_MUESTRAS) / 1000.0;
            acumMiliboltios = 0;
            contadorMuestras = 0;
        }
    }

    // 2. Evaluación y reporte (Cada 2 segundos)
    if (tiempoActual - ultimoReporte >= TIEMPO_REPORTE) {
        ultimoReporte = tiempoActual;

        // Evalúa si el cambio es superior a 100 mV (0.1 V) o si es la primera lectura
        if (ultimoVoltajeReportado < 0 || abs(voltajePromedioTemp - ultimoVoltajeReportado) >= PASO_MINIMO_V) {
            
            // Redondeo/escalonado a pasos exactos de 100 mV (0.1 V)
            float voltajeEscalonado = round(voltajePromedioTemp / PASO_MINIMO_V) * PASO_MINIMO_V;
            ultimoVoltajeReportado = voltajeEscalonado;

            // Mapeo e interpolación
            float voltajeLimpio = constrain(voltajeEscalonado, VOLTAJE_MIN, VOLTAJE_MAX);
            float proporcion = (voltajeLimpio - VOLTAJE_MIN) / (VOLTAJE_MAX - VOLTAJE_MIN);

            // Asignación de resultados finales
            resultado.voltaje = voltajeEscalonado;
            resultado.porcentaje = constrain(proporcion * 100.0, 0.0, 100.0);
            resultado.voltajeBateria = proporcion * (BATT_MAX - BATT_MIN) + BATT_MIN;
            resultado.adc_crudo = analogRead(PIN_ANALOG);
        }
    }

    return resultado;
}