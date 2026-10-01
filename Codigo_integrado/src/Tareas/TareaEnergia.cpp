#include "TareaEnergia.h"
#include "ModoEnergia.h"
#include <math.h>
#include <string.h>
#include <ctype.h>
#include "../Rtos/Rtos.h"
#include "../Rtos/Compartidos.h"
#include "../Sensores/Tension/Tension.h"
#include "../Sensores/Corriente/Corriente.h"
#include "../Sensores/Energia/SesionEnergia.h"

// Base: "main tension.cpp" (versión vigente). Sin temperatura/GPS/ISR (tareas propias).
namespace {
SesionEnergia sesion(Config::CAPACIDAD_PACK_AH, Config::TENSION_NOMINAL_PACK_V);
MedidaTension tension = {};
MedidaCorriente corriente = {};
bool lecturasDisponibles = false;
uint32_t ultimoReporteMs = 0;

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
            solicitarCalibracion();
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

void tarea(void*) {
    const char* errorConfig = Config::errorConfiguracion();
    if (errorConfig) {
        {
            BloqueoSerial b;
            Serial.print("ERROR CONFIGURACION: "); Serial.println(errorConfig);
        }
        vTaskSuspend(NULL); // No retornar: una tarea FreeRTOS no puede terminar.
    }
    if (usaTensionADC()) inicializarTension();
    ultimoReporteMs = millis();
    sesion.reiniciar(ultimoReporteMs);
    {
        BloqueoSerial b;
        if (calibraAlArrancar()) Serial.println("ACS real: estabilizando/calibrando SIN CARGA.");
        Serial.print("Modo seleccionado: "); Serial.println(nombreModo());
        Serial.println("Arranque PAUSADO. Ajustar fuentes y luego enviar iniciar.");
        imprimirAyuda();
    }

    TickType_t ultimo = xTaskGetTickCount();
    uint32_t ultimoLatidoMs = millis();
    for (;;) {
        if (usaTensionADC()) {
            tension = leerTensionCompleta();
            publicarTension(tension);
        }
        if (usaCorrienteADC()) corriente = leerCorriente();
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
        publicarEnergia(sesion.obtenerEstado());
        {
            BloqueoSerial b;
            if (b.ok) leerComandos();
        }
        if (millis() - ultimoReporteMs >= Config::INTERVALO_REPORTE_MS) {
            ultimoReporteMs = millis();
            BloqueoSerial b;
            if (b.ok) imprimirEstado();
        }
        if (HEARTBEAT_TAREAS && millis() - ultimoLatidoMs >= PERIODO_HEARTBEAT_MS) {
            ultimoLatidoMs = millis();
            logSerial("[Energia] latido core=%d stack_libre=%u\n", xPortGetCoreID(),
                      (unsigned)uxTaskGetStackHighWaterMark(NULL));
        }
        vTaskDelayUntil(&ultimo, pdMS_TO_TICKS(PERIODO_ENERGIA_MS));
    }
}
} // namespace

bool crearTareaEnergia() {
    if (!HABILITAR_TAREA_ENERGIA) return true;
    return xTaskCreatePinnedToCore(tarea, "Energia", STACK_ENERGIA, nullptr, PRIO_ENERGIA, nullptr, CORE_1) == pdPASS;
}
