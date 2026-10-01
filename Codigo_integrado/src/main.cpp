#include <Arduino.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

#include "Configuracion.h"
#include "Sensores/Tension/Tension.h"
#include "Sensores/Corriente/Corriente.h"
#include "Sensores/Energia/SesionEnergia.h"
#include "Sensores/GPS/GPS.h"
#include "Sensores/Temperatura/Temperatura.h"

SesionEnergia sesion(Config::CAPACIDAD_PACK_AH, Config::TENSION_NOMINAL_PACK_V);
MedidaTension tension = {};
MedidaCorriente corriente = {};
bool lecturasDisponibles = false;
const char* errorConfig = nullptr;
uint32_t ultimoReporteMs = 0;

// Sensores auxiliares de la rama original, habilitables en Configuracion.h.
GestorTemperatura sensorTemp1(25), sensorTemp2(33);
const uint8_t pinInductivo = 26;
volatile uint32_t inicioPulsoMs = 0, intervaloPulsoMs = 0;
volatile bool pulsoNuevo = false;
uint32_t ultimaSolicitudTempMs = 0;
bool temperaturaPendiente = false;
float temperatura1C = 0.0f, temperatura2C = 0.0f;

void IRAM_ATTR cuentaPulsos() {
    const uint32_t ahoraMs = millis();
    intervaloPulsoMs = ahoraMs - inicioPulsoMs;
    inicioPulsoMs = ahoraMs;
    pulsoNuevo = true;
}

bool usaTensionADC() {
    return Config::MODO == Config::ModoEnergia::FUENTES_ANALOGICAS ||
           Config::MODO == Config::ModoEnergia::SENSORES_REALES;
}
bool usaCorrienteADC() {
    return Config::MODO != Config::ModoEnergia::REFERENCIA_SIMULADA;
}
bool usaACSReal() {
    return Config::MODO == Config::ModoEnergia::SENSORES_REALES ||
           Config::MODO == Config::ModoEnergia::ACS_TENSION_FIJA;
}
const char* nombreModo() {
    switch (Config::MODO) {
        case Config::ModoEnergia::FUENTES_ANALOGICAS: return "FUENTES_ANALOGICAS";
        case Config::ModoEnergia::SENSORES_REALES: return "SENSORES_REALES";
        case Config::ModoEnergia::REFERENCIA_SIMULADA: return "REFERENCIA_SIMULADA";
        case Config::ModoEnergia::VOLTAJE_ADC_INYECTADO: return "VOLTAJE_ADC_INYECTADO";
        case Config::ModoEnergia::ACS_TENSION_FIJA: return "ACS_TENSION_FIJA";
    }
    return "DESCONOCIDO";
}

void imprimirAyuda() {
    Serial.println("Comandos: escribir y enviar con Enter (115200 baudios).");
    Serial.println("iniciar / i: acumular; pausar / p: medir sin acumular.");
    Serial.println("reiniciar / r: borrar acumulados y quedar PAUSADO.");
    Serial.println("estado: reporte; test: prueba virtual separada (en pausa).");
    Serial.println("calibrar: medir cero solo con ACS real, sin carga y en pausa.");
    Serial.println("ayuda: repetir estos comandos.");
}

void imprimirEstado() {
    Serial.print("Modo: "); Serial.print(nombreModo());
    Serial.print(" | Sesion: "); Serial.print(sesion.activa() ? "ACTIVA" : "PAUSADA");
    Serial.print(" | Adquisicion: ");
    Serial.println(lecturasDisponibles ? "LEYENDO" : "ESPERANDO MUESTRAS/CALIBRACION");
    if (usaTensionADC()) {
        Serial.print("GPIO"); Serial.print(Config::PIN_TENSION);
        Serial.print(": "); Serial.print(tension.voltajePinV, 3);
        Serial.print(" V | Tension: ");
        Serial.println(tension.lista ? "LECTURA LISTA" : "ESPERANDO MUESTRAS");
    }
    if (usaCorrienteADC()) {
        Serial.print("GPIO"); Serial.print(Config::PIN_CORRIENTE);
        Serial.print(": "); Serial.print(corriente.voltajePinV, 3);
        Serial.print(" V | Salida sensor: "); Serial.print(corriente.voltajeSensorV, 3);
        Serial.print(" V | Offset efectivo: "); Serial.print(corriente.offsetSensorV, 3);
        Serial.print(" V | Corriente: ");
        Serial.println(corriente.calibrando ? "CALIBRANDO (SIN CARGA)" :
                       corriente.lista ? "LECTURA LISTA" : "ESPERANDO MUESTRAS");
    }
    const EstadoEnergia& e = sesion.obtenerEstado();
    if (lecturasDisponibles) {
        Serial.print("Pack: "); Serial.print(e.voltajeV, 2);
        Serial.print(" V | Corriente: "); Serial.print(e.corrienteA, 3);
        Serial.print(" A | Potencia: "); Serial.print(e.potenciaW, 2); Serial.println(" W");
    } else {
        Serial.println("Esperando primeras mediciones o calibracion.");
    }
    Serial.print("Ah consumidos: "); Serial.print(e.ahConsumidos, 4);
    Serial.print(" | Ah restantes: "); Serial.println(e.ahRestantes, 4);
    Serial.print("Wh consumidos: "); Serial.print(e.whConsumidos, 2);
    Serial.print(" | Wh restantes: "); Serial.println(e.whRestantes, 2);
    Serial.print("SOC: "); Serial.print(e.porcentajeBateria, 2);
    Serial.print(" % | Tiempo integrado: ");
    Serial.print(sesion.segundosIntegrados(), 1); Serial.println(" s");
    Serial.println("--------------------------------------------");
}

void ejecutarPruebaPatron() {
    // Caso de aceptación FIJO del algoritmo: independiente de la calibración
    // que el usuario configure para los sensores de su vehículo.
    GestorEnergia prueba(17.0f, 48.0f);
    EstadoEnergia e = {};
    for (int i = 0; i < 3600; ++i) e = prueba.actualizar(48.0f, 8.5f, 1.0f);
    const bool ok = fabsf(e.potenciaW - 408.0f) < 0.05f &&
        fabsf(e.ahConsumidos - 8.5f) < 0.01f &&
        fabsf(e.ahRestantes - 8.5f) < 0.01f &&
        fabsf(e.whConsumidos - 408.0f) < 0.05f &&
        fabsf(e.whRestantes - 408.0f) < 0.05f &&
        fabsf(e.porcentajeBateria - 50.0f) < 0.05f;
    Serial.println("PRUEBA VIRTUAL: 48 V, 8.5 A, 3600 s simulados.");
    Serial.print("P: "); Serial.print(e.potenciaW, 2);
    Serial.print(" W | Ah consumidos/restantes: "); Serial.print(e.ahConsumidos, 4);
    Serial.print(" / "); Serial.println(e.ahRestantes, 4);
    Serial.print("Wh consumidos/restantes: "); Serial.print(e.whConsumidos, 2);
    Serial.print(" / "); Serial.print(e.whRestantes, 2);
    Serial.print(" | SOC: "); Serial.println(e.porcentajeBateria, 2);
    Serial.println(ok ? "PRUEBA PATRON: OK" : "PRUEBA PATRON: ERROR");
    Serial.println("Esta prueba no cambia los acumulados de tu sesion.");
}

void ejecutarComando(const char* comando) {
    const uint32_t ahoraMs = millis();
    if (!strcmp(comando, "iniciar") || !strcmp(comando, "i")) {
        if (!lecturasDisponibles) {
            Serial.println("Esperar primeras mediciones o terminar calibracion.");
        } else {
            sesion.iniciar(ahoraMs);
            Serial.println("ACTIVA: integrando valores medidos con tiempo real.");
        }
    } else if (!strcmp(comando, "pausar") || !strcmp(comando, "p")) {
        sesion.pausar(ahoraMs);
        Serial.println("PAUSADA: las mediciones siguen; el consumo queda congelado.");
    } else if (!strcmp(comando, "reiniciar") || !strcmp(comando, "r")) {
        sesion.reiniciar(ahoraMs);
        Serial.println("Acumulados en cero. Sesion PAUSADA.");
    } else if (!strcmp(comando, "estado")) {
        imprimirEstado();
    } else if (!strcmp(comando, "test")) {
        if (sesion.activa()) Serial.println("Primero enviar pausar.");
        else ejecutarPruebaPatron();
    } else if (!strcmp(comando, "calibrar")) {
        if (!usaACSReal()) {
            Serial.println("Modo con fuentes: offset fijo. Editarlo en Configuracion.h.");
        } else if (sesion.activa()) {
            Serial.println("Primero enviar pausar y quitar toda la carga.");
        } else {
            Serial.println("Calibracion iniciada: mantener corriente CERO.");
            recalibrarSensorCorriente();
            lecturasDisponibles = false;
            sesion.actualizar(0.0f, 0.0f, ahoraMs, false);
        }
    } else if (!strcmp(comando, "ayuda") || !strcmp(comando, "?")) {
        imprimirAyuda();
    } else {
        Serial.println("Comando desconocido. Enviar ayuda.");
    }
}

void leerComandos() {
    // Buffer fijo y lectura por caracteres: no usa String ni esperas por línea.
    static char buffer[24];
    static uint8_t longitud = 0;
    static bool desbordado = false;
    uint8_t presupuesto = 32;
    while (presupuesto-- && Serial.available()) {
        const char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (desbordado) Serial.println("Comando demasiado largo.");
            else if (longitud) {
                buffer[longitud] = '\0';
                ejecutarComando(buffer);
            }
            longitud = 0;
            desbordado = false;
        } else if (!desbordado) {
            if (longitud + 1 < sizeof(buffer))
                buffer[longitud++] = tolower(static_cast<unsigned char>(c));
            else desbordado = true;
        }
    }
}

void actualizarAuxiliares(uint32_t ahoraMs) {
    if (!Config::HABILITAR_SENSORES_AUXILIARES) return;
    if (temperaturaPendiente && ahoraMs - ultimaSolicitudTempMs >= 750) {
        temperatura1C = sensorTemp1.leerTemperatura(0);
        temperatura2C = sensorTemp2.leerTemperatura(0);
        temperaturaPendiente = false;
    }
    if (!temperaturaPendiente && ahoraMs - ultimaSolicitudTempMs >= 1000) {
        sensorTemp1.solicitarTemperaturas();
        sensorTemp2.solicitarTemperaturas();
        ultimaSolicitudTempMs = ahoraMs;
        temperaturaPendiente = true;
    }
    actualizarGPS();
    if (datosGPSActualizados()) {
        const DatosGPS gps = obtenerDatosGPS();
        Serial.print("GPS: "); Serial.print(gps.latitud, 6);
        Serial.print(", "); Serial.print(gps.longitud, 6);
        Serial.print(" | km/h: "); Serial.print(gps.velocidadKmH, 2);
        Serial.print(" | Sat: "); Serial.println(gps.satelites);
        Serial.print("Rumbo: ");
        if (gps.rumboValido) Serial.println(gps.rumboGrados, 1);
        else Serial.println("Sin rumbo");
        Serial.print("HDOP: "); Serial.println(gps.hdop);
        char fechaHora[40];
        snprintf(fechaHora, sizeof(fechaHora),
                 "HORA ARG: %02u/%02u/%04u %02u:%02u:%02u",
                 static_cast<unsigned>(gps.dia), static_cast<unsigned>(gps.mes),
                 static_cast<unsigned>(gps.anio), static_cast<unsigned>(gps.hora),
                 static_cast<unsigned>(gps.minuto), static_cast<unsigned>(gps.segundo));
        Serial.println(fechaHora);
    }
}

void setup() {
    Serial.begin(Config::BAUDIOS_MONITOR);
    errorConfig = Config::errorConfiguracion();
    if (errorConfig) {
        Serial.print("ERROR CONFIGURACION: "); Serial.println(errorConfig);
        return;
    }
    if (usaTensionADC()) inicializarTension();
    if (usaCorrienteADC()) {
        const bool calibrar = usaACSReal() && Config::CALIBRACION_CORRIENTE ==
            Config::CalibracionCorriente::AUTOMATICA_EN_CERO;
        inicializarSensorCorriente(calibrar);
        if (calibrar) Serial.println("ACS real: estabilizando/calibrando SIN CARGA.");
    }
    if (Config::HABILITAR_SENSORES_AUXILIARES) {
        sensorTemp1.inicializar();
        sensorTemp2.inicializar();
        sensorTemp1.solicitarTemperaturas();
        sensorTemp2.solicitarTemperaturas();
        ultimaSolicitudTempMs = millis();
        temperaturaPendiente = true;
        pinMode(pinInductivo, INPUT);
        attachInterrupt(digitalPinToInterrupt(pinInductivo), cuentaPulsos, RISING);
        inicializarGPS();
    }
    ultimoReporteMs = millis();
    sesion.reiniciar(ultimoReporteMs);
    Serial.print("Modo seleccionado: "); Serial.println(nombreModo());
    Serial.println("Arranque PAUSADO. Ajustar fuentes y luego enviar iniciar.");
    imprimirAyuda();
}

void loop() {
    if (errorConfig) return;
    if (usaTensionADC()) tension = leerTensionCompleta();
    if (usaCorrienteADC()) {
        actualizarSensorCorriente();
        corriente = obtenerMedidaCorriente();
    }
    const uint32_t ahoraMs = millis();
    bool tensionDisponible = true, corrienteDisponible = true;
    float packV = Config::TENSION_REFERENCIA_V;
    float amperes = Config::CORRIENTE_REFERENCIA_A;
    if (usaTensionADC()) {
        tensionDisponible = tension.lista;
        packV = tension.voltajeBateria;
    }
    if (usaCorrienteADC()) {
        corrienteDisponible = corriente.lista && !corriente.calibrando;
        amperes = corriente.corrienteA;
    }
    // Solo espera el primer promedio o una calibración pedida por el usuario.
    // El valor de la señal no detiene la integración ni se convierte en cero.
    lecturasDisponibles = tensionDisponible && corrienteDisponible;
    sesion.actualizar(packV, amperes, ahoraMs, lecturasDisponibles);
    leerComandos();
    actualizarAuxiliares(ahoraMs);
    if (ahoraMs - ultimoReporteMs >= Config::INTERVALO_REPORTE_MS) {
        ultimoReporteMs = ahoraMs;
        imprimirEstado();
    }
}
