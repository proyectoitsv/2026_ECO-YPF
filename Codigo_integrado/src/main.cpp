#include <Arduino.h>
#include <math.h>

#include "Sensores/Corriente/Corriente.h"
#include "Sensores/Energia/Energia.h"
#include "Sensores/GPS/GPS.h"
#include "Sensores/Inductivo(velocidad)/Inductivo.h"
#include "Sensores/Temperatura/Temperatura.h"
#include "Sensores/Tension/Tension.h"

// Cambiar solo esta línea para elegir la etapa de prueba:
// 0: 48 V y salida ACS758 de 2,840 V simuladas en software (8,5 A).
// 1: 48 V fijos y 2,840 V aplicados al GPIO35; offset nominal de 2,500 V.
// 2: 48 V fijos y ACS758 real en GPIO35, calibrado sin corriente al arrancar.
// 3: ACS758 real en GPIO35 y módulo de tensión real en GPIO34.
enum class ModoEnergia : uint8_t {
    REFERENCIA_SIMULADA = 0,
    VOLTAJE_ADC_INYECTADO = 1,
    ACS_TENSION_FIJA = 2,
    SENSORES_REALES = 3
};
constexpr ModoEnergia MODO_ENERGIA = ModoEnergia::REFERENCIA_SIMULADA;

const float TENSION_PACK_FIJA_V = 48.0f;
const float SALIDA_ACS_SIMULADA_V = 2.840f;
const float OFFSET_ACS_NOMINAL_V = 2.500f;
const uint32_t INTERVALO_REPORTE_MS = 1000;

// Los cuatro acumuladores de 12 V están en serie: el pack sigue siendo 17 Ah.
GestorEnergia energiaVehiculo(17.0f, 48.0f);
EstadoEnergia ultimoEstadoEnergia = {};
bool estadoEnergiaDisponible = false;
uint32_t ultimoTiempoEnergiaMs = 0;
uint32_t ultimoReporteEnergiaMs = 0;

// Sensores existentes del proyecto integrado.
GestorTemperatura sensorTemp1(25);
GestorTemperatura sensorTemp2(33);
DatosGPS datosGPS;
const int pinSensorInductivo = 26;
volatile uint32_t tiempoInicioPulsoMs = 0;
volatile uint32_t tiempoEntrePulsosMs = 0;
volatile bool hayPulsoNuevo = false;

float temperatura1C = 0.0f;
float temperatura2C = 0.0f;
float temperaturaPromedioC = 0.0f;
uint32_t ultimaSolicitudTemperaturaMs = 0;
bool conversionTemperaturaPendiente = false;

void IRAM_ATTR cuentaPulsos() {
    const uint32_t ahoraMs = millis();
    tiempoEntrePulsosMs = ahoraMs - tiempoInicioPulsoMs;
    tiempoInicioPulsoMs = ahoraMs;
    hayPulsoNuevo = true;
}

void imprimirEstadoEnergia(const EstadoEnergia& estado) {
    Serial.print("Pack: "); Serial.print(estado.voltajeV, 2);
    Serial.print(" V | Corriente: "); Serial.print(estado.corrienteA, 2);
    Serial.print(" A | Potencia: "); Serial.print(estado.potenciaW, 2);
    Serial.println(" W");

    Serial.print("Ah consumidos: "); Serial.print(estado.ahConsumidos, 4);
    Serial.print(" | Ah restantes: "); Serial.println(estado.ahRestantes, 4);
    Serial.print("Wh consumidos: "); Serial.print(estado.whConsumidos, 2);
    Serial.print(" | Wh restantes: "); Serial.println(estado.whRestantes, 2);
    Serial.print("Bateria estimada: ");
    Serial.print(estado.porcentajeBateria, 2); Serial.println(" %");
}

void ejecutarPruebaPatron() {
    // Prueba acelerada: 3600 intervalos simulados de 1 s, sin esperar 1 hora.
    GestorEnergia prueba(17.0f, 48.0f);
    const float corrientePruebaA = corrienteDesdeVoltajeSensor(
        SALIDA_ACS_SIMULADA_V, OFFSET_ACS_NOMINAL_V);
    EstadoEnergia resultado = {};
    for (int segundo = 0; segundo < 3600; ++segundo) {
        resultado = prueba.actualizar(TENSION_PACK_FIJA_V, corrientePruebaA, 1.0f);
    }

    Serial.println("=== PRUEBA PATRON: 48 V, 2.840 V ACS, 1 h ===");
    imprimirEstadoEnergia(resultado);
    const bool correcto =
        fabsf(resultado.potenciaW - 408.0f) < 0.05f &&
        fabsf(resultado.ahConsumidos - 8.5f) < 0.01f &&
        fabsf(resultado.ahRestantes - 8.5f) < 0.01f &&
        fabsf(resultado.whConsumidos - 408.0f) < 0.05f &&
        fabsf(resultado.whRestantes - 408.0f) < 0.05f &&
        fabsf(resultado.porcentajeBateria - 50.0f) < 0.05f;
    Serial.println(correcto ? "PRUEBA PATRON: OK" : "PRUEBA PATRON: ERROR");
    Serial.println("============================================");
}

void actualizarTemperaturas(uint32_t ahoraMs) {
    // DallasTemperature se configuró sin espera bloqueante. Cada conversión
    // de 12 bits puede tardar hasta 750 ms; se lee después de ese intervalo.
    if (conversionTemperaturaPendiente &&
        ahoraMs - ultimaSolicitudTemperaturaMs >= 750) {
        temperatura1C = sensorTemp1.leerTemperatura(0);
        temperatura2C = sensorTemp2.leerTemperatura(0);
        temperaturaPromedioC = (temperatura1C + temperatura2C) / 2.0f;
        conversionTemperaturaPendiente = false;
    }

    if (!conversionTemperaturaPendiente &&
        ahoraMs - ultimaSolicitudTemperaturaMs >= 1000) {
        sensorTemp1.solicitarTemperaturas();
        sensorTemp2.solicitarTemperaturas();
        ultimaSolicitudTemperaturaMs = ahoraMs;
        conversionTemperaturaPendiente = true;
    }
}

void imprimirGPS() {
    Serial.print("LAT: "); Serial.print(datosGPS.latitud, 6);
    Serial.print(" | LONG: "); Serial.println(datosGPS.longitud, 6);
    Serial.print("SPEED: "); Serial.print(datosGPS.velocidadKmH, 2);
    Serial.print(" km/h | RUMBO: ");
    if (datosGPS.rumboValido) {
        Serial.print(datosGPS.rumboGrados, 1);
        Serial.println(" grados");
    } else {
        Serial.println("Sin rumbo");
    }
    Serial.print("HDOP: "); Serial.print(datosGPS.hdop);
    Serial.print(" | Satelites: "); Serial.println(datosGPS.satelites);

    char bufferFechaHora[30];
    snprintf(bufferFechaHora, sizeof(bufferFechaHora),
             "HORA ARG: %04d/%02d/%02d %02d:%02d:%02d",
             datosGPS.anio, datosGPS.mes, datosGPS.dia,
             datosGPS.hora, datosGPS.minuto, datosGPS.segundo);
    Serial.println(bufferFechaHora);
}

void setup() {
    Serial.begin(115200);
    ejecutarPruebaPatron();

    if (MODO_ENERGIA != ModoEnergia::REFERENCIA_SIMULADA) {
        const bool calibrarOffset =
            MODO_ENERGIA == ModoEnergia::ACS_TENSION_FIJA ||
            MODO_ENERGIA == ModoEnergia::SENSORES_REALES;
        if (calibrarOffset) {
            Serial.println("Calibrando ACS758: mantener corriente en 0 A...");
        }
        inicializarSensorCorriente(calibrarOffset);
        Serial.print("Offset ACS758: ");
        Serial.print(obtenerOffsetSensorV(), 3);
        Serial.println(" V en salida del sensor");
        Serial.println("Corriente GPIO35 (ADC1); verificar acondicionamiento de 5 V.");
    }

    if (MODO_ENERGIA == ModoEnergia::SENSORES_REALES) {
        pinMode(34, INPUT);  // Módulo Tension existente.
    }

    sensorTemp1.inicializar();
    sensorTemp2.inicializar();
    sensorTemp1.solicitarTemperaturas();
    sensorTemp2.solicitarTemperaturas();
    ultimaSolicitudTemperaturaMs = millis();
    conversionTemperaturaPendiente = true;

    pinMode(pinSensorInductivo, INPUT);
    attachInterrupt(digitalPinToInterrupt(pinSensorInductivo), cuentaPulsos, RISING);
    inicializarGPS();

    // La medición acumulada comienza tras calibración e inicialización.
    ultimoTiempoEnergiaMs = millis();
    ultimoReporteEnergiaMs = ultimoTiempoEnergiaMs;
    Serial.print("Modo energia: ");
    Serial.println(static_cast<int>(MODO_ENERGIA));
}

void loop() {
    float voltajePackV = TENSION_PACK_FIJA_V;
    float corrienteA = corrienteDesdeVoltajeSensor(
        SALIDA_ACS_SIMULADA_V, OFFSET_ACS_NOMINAL_V);
    bool entradaLista = true;

    if (MODO_ENERGIA != ModoEnergia::REFERENCIA_SIMULADA) {
        actualizarSensorCorriente();
        entradaLista = sensorCorrienteListo();
        if (entradaLista) corrienteA = obtenerCorrienteA();
    }

    if (MODO_ENERGIA == ModoEnergia::SENSORES_REALES) {
        const MedidaTension medida = leerTensionCompleta();
        voltajePackV = medida.voltajeBateria;
        entradaLista = entradaLista && voltajePackV > 0.0f;
    }

    const uint32_t ahoraEnergiaMs = millis();
    if (entradaLista) {
        const float dtSegundos =
            static_cast<float>(ahoraEnergiaMs - ultimoTiempoEnergiaMs) / 1000.0f;
        ultimoTiempoEnergiaMs = ahoraEnergiaMs;
        ultimoEstadoEnergia = energiaVehiculo.actualizar(
            voltajePackV, corrienteA, dtSegundos);
        estadoEnergiaDisponible = true;
    } else {
        // No atribuir tiempo anterior a una lectura todavía no disponible.
        ultimoTiempoEnergiaMs = ahoraEnergiaMs;
    }

    actualizarTemperaturas(millis());
    actualizarGPS();
    if (datosGPSActualizados()) {
        datosGPS = obtenerDatosGPS();
        imprimirGPS();
    }

    const uint32_t ahoraReporteMs = millis();
    if (ahoraReporteMs - ultimoReporteEnergiaMs >= INTERVALO_REPORTE_MS) {
        ultimoReporteEnergiaMs = ahoraReporteMs;
        if (estadoEnergiaDisponible) {
            imprimirEstadoEnergia(ultimoEstadoEnergia);
            if (MODO_ENERGIA != ModoEnergia::REFERENCIA_SIMULADA) {
                Serial.print("GPIO35: ");
                Serial.print(obtenerPromedioADCmV());
                Serial.print(" mV | Salida ACS: ");
                Serial.print(obtenerSalidaSensorV(), 3);
                Serial.println(" V");
            }
        } else {
            Serial.println("Esperando lecturas fisicas de corriente/tension...");
        }
        Serial.println("--------------------------------------------");
    }
}
