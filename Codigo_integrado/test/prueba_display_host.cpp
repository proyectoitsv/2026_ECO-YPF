// Prueba de Energía -> cola -> tarea Display -> bytes UART, sin hardware.
// La cola y el planificador son simulados; el firmware real se compila con PIO.
#include <assert.h>
#include <string.h>
#include <deque>
#include <iostream>
#include <limits>
#include <vector>
#include "../src/Tareas/TareaDisplay.h"
#include "../src/Display/Display_COM11441.h"
// Permite ejecutar el productor y sus comandos reales sin iniciar su bucle ADC.
#include "../src/Tareas/TareaEnergia.cpp"

// La tarea Energia ahora enlaza también su memoria, aunque esta prueba solo
// ejecute el productor del display y no arranque el bucle de adquisición.
namespace NvsSimulada {
std::map<std::string, std::vector<uint8_t>> datos;
unsigned escrituras = 0;
bool fallaApertura = false, fallaEscritura = false;
}
HardwareSerial Serial;
HardwareSerial Serial1;
SemaphoreHandle_t mtxSerial = reinterpret_cast<void*>(1);
static uint32_t relojMs = 0;
uint32_t millis() { return relojMs; }
int64_t esp_timer_get_time() { return int64_t(relojMs)*1000; }
BaseType_t xSemaphoreTake(SemaphoreHandle_t, TickType_t) { return pdTRUE; }
BaseType_t xSemaphoreGive(SemaphoreHandle_t) { return pdTRUE; }
void vTaskDelay(TickType_t t) { relojMs += t; }
void vTaskDelayUntil(TickType_t* t, TickType_t d) { *t += d; relojMs = *t; }
TickType_t xTaskGetTickCount() { return relojMs; }
int xPortGetCoreID() { return 0; }
UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t) { return 1024; }
struct FinPrueba {};
void vTaskSuspend(TaskHandle_t) { throw FinPrueba{}; }

static void (*consumidorDisplay)(void*) = nullptr;
BaseType_t xTaskCreatePinnedToCore(void (*f)(void*), const char* nombre,
    uint32_t, void*, UBaseType_t, TaskHandle_t*, BaseType_t core) {
    assert(std::string(nombre) == "Display" && core == CORE_0);
    consumidorDisplay = f;
    return pdPASS;
}

struct ColaSimulada {
    size_t tamano = 0;
    bool llena = false;
    std::vector<uint8_t> dato;
};
static ColaSimulada cola;
struct Evento { uint32_t ms; EstadoDisplay estado; };
static std::deque<Evento> guion;
static uint32_t origen, limite;
struct Salida { uint32_t ms; std::vector<uint8_t> bytes; };
static std::vector<Salida> salidas;
static size_t bytesVistos = 0;
static bool ejecutandoConsumidor = false;
QueueHandle_t xQueueCreate(UBaseType_t longitud, UBaseType_t tamano) {
    assert(longitud == 1 && tamano == sizeof(EstadoDisplay));
    cola.tamano = tamano; cola.dato.resize(tamano);
    return &cola;
}
BaseType_t xQueueOverwrite(QueueHandle_t q, const void* dato) {
    memcpy(q->dato.data(), dato, q->tamano); q->llena = true;
    return pdPASS;
}
BaseType_t xQueueReceive(QueueHandle_t q, void* dato, TickType_t espera) {
    if (ejecutandoConsumidor) {
        const uint32_t relativo = relojMs - origen;
        if (Serial1.bytes.size() != bytesVistos) {
            salidas.push_back({relativo, std::vector<uint8_t>(
                Serial1.bytes.begin() + bytesVistos, Serial1.bytes.end())});
            bytesVistos = Serial1.bytes.size();
        }
        if (!q->llena) {
            assert(espera > 0 && espera <= pdMS_TO_TICKS(3000));
            if (!guion.empty() && guion.front().ms - relativo <= espera) {
                relojMs = origen + guion.front().ms;
                xQueueOverwrite(q, &guion.front().estado);
                guion.pop_front();
            } else {
                relojMs += espera;
                if (relojMs - origen > limite) throw FinPrueba{};
                return 0;
            }
        }
    }
    if (!q->llena) return 0;
    memcpy(dato, q->dato.data(), q->tamano); q->llena = false;
    return pdTRUE;
}

// La adquisición no se ejecuta en esta prueba: se conserva el cálculo real.
void inicializarTension() {}
MedidaTension leerTensionCompleta() { return {}; }
MedidaCorriente leerCorriente() { return {}; }
static DatosVelocidad velocidadSimulada = {};
DatosVelocidad leerVelocidad() { return velocidadSimulada; }
void publicarTension(const MedidaTension&) {}
void publicarEnergia(const EstadoEnergia&) {}
void publicarInfoEnergia(const InfoEnergiaRegistro&) {}
bool procesarComandoRegistro(const char*) { return false; }
void solicitarCalibracion() {}

static EstadoDisplay recibir() {
    EstadoDisplay e = {};
    assert(recibirEstadoDisplay(e, 0)); return e;
}

int main() {
    assert(!publicarEstadoDisplay({100.0f, true, false, 0, 0.0f, false}));
    assert(inicializarColaDisplay() && inicializarColaDisplay());
    sesion.reiniciar(0);
    lecturasDisponibles = false;
    actualizarDisplay(0, true);
    assert(!recibir().lecturaDisponible);
    lecturasDisponibles = true;
    actualizarDisplay(0, true);
    assert(recibir().porcentajeBateria == 100.0f);

    // Una hora virtual real del módulo Energía produce SOC 50 %.
    sesion.iniciar(0);
    sesion.actualizar(48.0f, 8.5f, 0, true);
    const EstadoEnergia energia = sesion.actualizar(48.0f, 8.5f, 3600000, true);
    assert(fabsf(energia.porcentajeBateria - 50.0f) < 0.001f);
    assert(fabsf(energia.ahConsumidos - 8.5f) < 0.001f);
    assert(fabsf(energia.whConsumidos - 408.0f) < 0.001f);
    actualizarDisplay(0, true);
    assert(fabsf(recibir().porcentajeBateria - 50.0f) < 0.001f);

    // El modo manual persiste aunque Energía publique otro estado periódico.
    ejecutarComando("display 1234");
    actualizarDisplay(Config::INTERVALO_ENVIO_SOC_MS, false);
    EstadoDisplay e = recibir();
    assert(e.modoPrueba && e.numeroPrueba == 1234);
    ejecutarComando("display 10000");
    ejecutarComando("display 12abc");
    assert(modoPruebaDisplay && numeroPruebaDisplay == 1234);
    ejecutarComando("display auto");
    assert(!recibir().modoPrueba);
    assert(sesion.activa() && sesion.obtenerEstado().porcentajeBateria == 50.0f);

    // El productor transmite la velocidad de rueda sin depender del SOC.
    velocidadSimulada.kmh = 27.1f;
    velocidadSimulada.capturaUs = 1000;
    actualizarDisplay(0, true);
    e = recibir();
    assert(e.velocidadDisponible && e.velocidadKmH == 27.1f);
    const EstadoDisplay inicial = {50.2f, true, false, 0, 27.1f, true};
    assert(crearTareaDisplay() && consumidorDisplay);
    // Dos vueltas, datos durante encendido/apagado y cruce del contador de ticks.
    for (uint32_t inicio : {0u, UINT32_MAX - 9000u}) {
        relojMs = inicio;
        origen = inicio + Config::ESTABILIZACION_DISPLAY_MS;
        limite = 32000;
        salidas.clear(); Serial1.bytes.clear(); bytesVistos = 0;
        publicarEstadoDisplay({60.0f, true, false, 0, 10.0f, true});
        publicarEstadoDisplay(inicial); // La cola conserva solo el último dato.
        guion = {
            {3500, {50.2f, true, false, 0, 28.2f, true}},
            {4000, {50.2f, true, false, 0, 28.2f, true}}, // No reenviar.
            {6000, {49.49f, true, false, 0, 9.0f, true}}, // Recibir en blanco.
            {8500, {49.49f, true, false, 0, 0.0f, false}},
            {11000, {49.49f, true, false, 0, 0.0f, true}},
            {19500, {49.49f, true, true, 1234, 0.0f, true}},
            {20000, {49.49f, true, false, 0, 99.9f, true}},
            {24500, {49.49f, true, false, 0, 100.0f, true}},
            {29500, {101.0f, true, false, 0, 0.0f, true}}
        };
        ejecutandoConsumidor = true;
        try { consumidorDisplay(nullptr); } catch (const FinPrueba&) {}
        ejecutandoConsumidor = false;
        assert(Serial1.baud == Config::BAUDIOS_DISPLAY && Serial1.config == SERIAL_8N1);
        assert(Serial1.tx == Config::PIN_TX_DISPLAY && Serial1.rx == Config::PIN_RX_DISPLAY);
        const std::vector<uint32_t> tiempos = {
            0, 3000, 3500, 5000, 8000, 8500, 10000, 13000, 16000,
            19000, 19500, 20000, 21000, 24000, 24500, 26000, 29000, 29500, 32000
        };
        const std::vector<std::string> textos = {
            "", "271-", "282-", "", " 90-", "----", "", "  49", "",
            " 00-", "1234", "999-", "", "999-", "----", "", "  49", " 100", ""
        };
        assert(salidas.size() == tiempos.size());
        for (size_t i = 0; i < salidas.size(); ++i) {
            assert(salidas[i].ms == tiempos[i]);
            const auto& bytes = salidas[i].bytes;
            if (i == 0) {
                assert((bytes == std::vector<uint8_t>{0x76, 0x7A, Config::BRILLO_DISPLAY}));
            } else if (textos[i].empty()) {
                assert((bytes == std::vector<uint8_t>{0x76}));
            } else {
                assert(bytes.size() == 8 && bytes[0] == 0x77);
                const bool velocidad = textos[i].back() == '-' && textos[i] != "----";
                assert(bytes[1] == (velocidad ? 0x02 : 0));
                assert(bytes[2] == 0x79 && bytes[3] == 0);
                assert(std::string(bytes.begin() + 4, bytes.end()) == textos[i]);
            }
        }
    }
    Serial1.bytes.clear();
    DisplayCOM11441 driver(Serial1, Config::PIN_TX_DISPLAY, Config::PIN_RX_DISPLAY);
    driver.showText("1");
    assert(std::string(Serial1.bytes.begin() + 4, Serial1.bytes.end()) == "1   ");
    std::cout << "OK: velocidad de rueda -> cola -> UART, decimal y guion, secuencia de 16 s, "
                 "actualizacion visible, bateria, modo manual, limites y desbordamiento del reloj.\n";
}
