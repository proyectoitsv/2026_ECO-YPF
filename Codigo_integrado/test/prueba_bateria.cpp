// Ejecuta la tarea Energia, los dos módulos ADC y la memoria de producción.
// Solo se simulan el reloj, el hardware ADC/UART, FreeRTOS y la API Preferences.
// No acredita el comportamiento eléctrico de una placa ni de la flash real.
#include <assert.h>
#include <iostream>
#include "../src/Tareas/TareaEnergia.cpp"

namespace NvsSimulada {
std::map<std::string, std::vector<uint8_t>> datos;
unsigned escrituras = 0;
bool fallaApertura = false, fallaEscritura = false;
}
HardwareSerial Serial;
SemaphoreHandle_t mtxSerial = reinterpret_cast<void*>(1);
static uint32_t reloj = 0, fin = 0;
static float cargaA = 0.0f;
static bool negativos = false, solicitudCalibracion = false, serialTomado = false;
static MedidaCorriente corrienteCompartida = {};
static InfoEnergiaRegistro infoPublicada = {};
static EstadoDisplay displayPublicado = {};
struct FinSimulacion {};
uint32_t millis() { return reloj; }
int64_t esp_timer_get_time() { return int64_t(reloj) * 1000; }
uint32_t analogReadMilliVolts(int pin) {
    if (pin == Config::PIN_TENSION) return 550; // 48 V según la escala actual.
    // Cero al arrancar; después carga de prueba. También ensaya +/-5 mV.
    if (reloj < 4000) return 2500;
    if (negativos) return 2460; // -1 A.
    if (cargaA == 0.0f) return (reloj / 75) % 2 ? 2495 : 2505;
    return uint32_t(2500.0f + cargaA * 40.0f);
}
uint16_t analogRead(int) { return 682; }
BaseType_t xSemaphoreTake(SemaphoreHandle_t, TickType_t) {
    assert(!serialTomado); serialTomado = true; return pdTRUE;
}
BaseType_t xSemaphoreGive(SemaphoreHandle_t) {
    assert(serialTomado); serialTomado = false; return pdTRUE;
}
BaseType_t xTaskCreatePinnedToCore(void(*)(void*), const char*, uint32_t,
    void*, UBaseType_t, TaskHandle_t*, BaseType_t) { return pdPASS; }
TickType_t xTaskGetTickCount() { return reloj; }
int xPortGetCoreID() { return 1; }
UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t) { return 2048; }
void vTaskSuspend(TaskHandle_t) { assert(false); }
void vTaskDelay(TickType_t) { assert(false); }
void vTaskDelayUntil(TickType_t* ultimo, TickType_t paso) {
    // Entre dos vueltas de Energia, Corriente continúa cada 5 ms como en placa.
    for (TickType_t i = 0; i < paso; i += Config::INTERVALO_CORRIENTE_MS) {
        reloj += Config::INTERVALO_CORRIENTE_MS;
        if (consumirSolicitudCalibracion()) recalibrarSensorCorriente();
        actualizarSensorCorriente();
        corrienteCompartida = obtenerMedidaCorriente();
    }
    *ultimo += paso;
    if (reloj >= fin) throw FinSimulacion{};
}
void publicarTension(const MedidaTension&) {}
MedidaCorriente leerCorriente() { return corrienteCompartida; }
void publicarInfoEnergia(const InfoEnergiaRegistro& i) { infoPublicada = i; }
bool publicarEstadoDisplay(const EstadoDisplay& e) { displayPublicado = e; return true; }
bool procesarComandoRegistro(const char*) { return false; } // SD ausente; no interviene.
void solicitarCalibracion() { solicitudCalibracion = true; }
bool consumirSolicitudCalibracion() {
    const bool r = solicitudCalibracion; solicitudCalibracion = false; return r;
}
static void arrancar(uint32_t duracionMs) {
    reloj = 0; fin = duracionMs;
    sesion = SesionEnergia(Config::CAPACIDAD_PACK_AH, Config::TENSION_NOMINAL_PACK_V);
    memoriaBateria = MemoriaBateria{};
    inicioAutomaticoPendiente = true;
    permitirGuardado = errorGuardadoAnunciado = lecturasDisponibles = false;
    ultimoAhGuardado = ultimoWhGuardado = -1.0f;
    ultimoEnvioDisplayMs = 0; segmentoEnergia = 0;
    corrienteCompartida = {}; infoPublicada = {}; Serial.texto.clear();
    inicializarSensorCorriente(true);
    try { tarea(nullptr); } catch (const FinSimulacion&) {}
    assert(!serialTomado);
}
static void continuar(uint32_t ms) {
    // Avanza con las mismas entradas y el mismo acumulador, sin reiniciar.
    const uint32_t hasta = reloj + ms;
    while (reloj < hasta) {
        reloj += 20;
        const float i = negativos ? -1.0f : cargaA;
        sesion.actualizar(48.0f, i, reloj, true);
        guardarBateria(reloj);
    }
}
static void comando(const char* texto) {
    BloqueoSerial b; assert(b.ok); ejecutarComando(texto);
}
static bool cerca(float a, float b, float tolerancia = 0.0001f) {
    return fabsf(a - b) < tolerancia;
}
int main() {
    // Primera carga sin comandos ni tarjeta: no integra la espera de calibración.
    cargaA = 8.5f;
    arrancar(12000);
    assert(sesion.activa() && lecturasDisponibles && infoPublicada.activa);
    assert(Serial.texto.find("LISTO") != std::string::npos);
    assert(cerca(corriente.offsetSensorV, 2.5f));
    assert(sesion.segundosIntegrados() > 8.0 && sesion.segundosIntegrados() < 9.1);
    assert(cerca(sesion.obtenerEstado().ahConsumidos, 8.5f * 8.0f / 3600.0f, 0.0003f));
    assert(NvsSimulada::escrituras == 3); // Inicial, segundo 5, segundo 10.
    const float antesDelCorte = sesion.obtenerEstado().ahConsumidos;
    MemoriaBateria comprobacion;
    assert(comprobacion.iniciar());
    float ahGuardado = 0, whGuardado = 0;
    assert(comprobacion.cargar(ahGuardado, whGuardado) == MemoriaBateria::Carga::RECUPERADA);
    assert(antesDelCorte > ahGuardado);
    assert(antesDelCorte - ahGuardado <= 8.5f * 5.0f / 3600.0f);
    // Corte y arranque: recupera el guardado, sin sumar el tiempo apagado/calibrando.
    arrancar(3600);
    assert(sesion.activa() && cerca(sesion.obtenerEstado().ahConsumidos, ahGuardado));
    assert(cerca(sesion.obtenerEstado().whConsumidos, whGuardado));
    assert(displayPublicado.lecturaDisponible && displayPublicado.porcentajeBateria < 100);
    assert(Serial.texto.find("RECUPERADA") != std::string::npos);
    assert(sesion.segundosIntegrados() < 0.6);
    // Carga y corriente negativas no recuperan Ah/Wh; ruido +/-5 mV queda en cero.
    negativos = true;
    arrancar(8000);
    assert(corriente.corrienteA < 0 && sesion.obtenerEstado().potenciaW < 0);
    assert(cerca(sesion.obtenerEstado().ahConsumidos, ahGuardado));
    assert(cerca(sesion.obtenerEstado().whConsumidos, whGuardado));
    negativos = false; cargaA = 0;
    const unsigned escriturasEnReposo = NvsSimulada::escrituras;
    arrancar(15000);
    assert(corriente.corrienteA == 0 && cerca(sesion.obtenerEstado().ahConsumidos, ahGuardado));
    assert(NvsSimulada::escrituras == escriturasEnReposo);
    // Un guardado fallido conserva el anterior y se reintenta, sin bucle de bloqueo.
    cargaA = 8.5f; continuar(1000);
    NvsSimulada::fallaEscritura = true;
    continuar(6000);
    assert(errorGuardadoAnunciado && sesion.activa());
    NvsSimulada::fallaEscritura = false; continuar(5000);
    assert(!errorGuardadoAnunciado);
    // Reset explícito persiste inmediatamente y mantiene la integración activa.
    comando("bateria llena");
    assert(sesion.activa() && sesion.obtenerEstado().porcentajeBateria == 100.0f);
    assert(comprobacion.cargar(ahGuardado, whGuardado) == MemoriaBateria::Carga::RECUPERADA);
    assert(ahGuardado == 0 && whGuardado == 0);
    arrancar(3600);
    assert(sesion.obtenerEstado().porcentajeBateria == 100.0f && sesion.activa());
    // Pausa manual no se deshace sola; iniciar la reanuda sin resetear.
    comando("pausar");
    assert(!sesion.activa() && !inicioAutomaticoPendiente);
    comando("iniciar"); assert(sesion.activa());
    // Una batería a mitad de carga conserva exactamente el 50% al encender.
    assert(comprobacion.guardar(8.5f, 408.0f));
    arrancar(3600);
    assert(cerca(sesion.obtenerEstado().porcentajeBateria, 50.0f));
    assert(cerca(sesion.obtenerEstado().ahRestantes, 8.5f));
    assert(cerca(sesion.obtenerEstado().whRestantes, 408.0f));
    // No se aceptan contadores NaN/negativos ni registros de longitud inválida.
    assert(!comprobacion.guardar(NAN, 0) && !comprobacion.guardar(-1, 0));
    NvsSimulada::datos["eco-bateria/consumo"] = {1, 2};
    const unsigned antesError = NvsSimulada::escrituras;
    arrancar(8000);
    assert(!permitirGuardado && NvsSimulada::escrituras == antesError);
    assert(Serial.texto.find("ERROR memoria bateria") != std::string::npos);
    comando("bateria llena"); assert(permitirGuardado);
    NvsSimulada::fallaApertura = true;
    arrancar(6000); assert(!permitirGuardado && sesion.activa());
    // millis() desbordado: solo integra la diferencia de contadores sin signo.
    SesionEnergia desborde(17, 48);
    desborde.restaurar(UINT32_MAX - 1000, 8.5f, 408);
    desborde.iniciar(UINT32_MAX - 1000);
    desborde.actualizar(48, 8.5f, UINT32_MAX - 1000, true);
    desborde.actualizar(48, 8.5f, 999, true);
    assert(cerca(desborde.obtenerEstado().ahConsumidos, 8.5f + 17.0f / 3600));
    std::cout << "OK: arranque automatico, calibracion, consumo, recuperacion, corte, reset, "
                 "reposo, negativos, fallos de memoria y desbordamiento de millis.\n";
}
