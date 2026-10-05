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

HardwareSerial Serial;
HardwareSerial Serial1;
SemaphoreHandle_t mtxSerial = reinterpret_cast<void*>(1);
static uint32_t relojMs = 0;
uint32_t millis() { return relojMs; }
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
static std::deque<EstadoDisplay> guion;
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
    if (!q->llena && ejecutandoConsumidor && !guion.empty()) {
        xQueueOverwrite(q, &guion.front()); guion.pop_front();
    }
    if (!q->llena) {
        if (ejecutandoConsumidor) {
            assert(espera == portMAX_DELAY); throw FinPrueba{};
        }
        return 0;
    }
    memcpy(dato, q->dato.data(), q->tamano); q->llena = false;
    return pdTRUE;
}

// La adquisición no se ejecuta en esta prueba: se conserva el cálculo real.
void inicializarTension() {}
MedidaTension leerTensionCompleta() { return {}; }
MedidaCorriente leerCorriente() { return {}; }
void publicarTension(const MedidaTension&) {}
void publicarEnergia(const EstadoEnergia&) {}
void solicitarCalibracion() {}

static EstadoDisplay recibir() {
    EstadoDisplay e = {};
    assert(recibirEstadoDisplay(e, 0)); return e;
}

int main() {
    assert(!publicarEstadoDisplay({100.0f, true, false, 0}));
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

    // Dos envíos sin consumidor: queda solo el más reciente.
    publicarEstadoDisplay({60.0f, true, false, 0});
    publicarEstadoDisplay({50.2f, true, false, 0});
    // Se inicia el consumidor real y se alimenta una secuencia controlada.
    guion = {
        {50.4f, true, false, 0}, {49.49f, true, false, 0},
        {49.49f, true, true, 1234}, {49.49f, true, true, 1234},
        {49.49f, true, false, 0}, {49.49f, false, false, 0},
        {49.49f, false, false, 0}, {0.1f, true, false, 0},
        {101.0f, true, false, 0}, {-1.0f, true, false, 0},
        {std::numeric_limits<float>::quiet_NaN(), true, false, 0},
        {0.0f, false, true, 42}
    };
    assert(crearTareaDisplay() && consumidorDisplay);
    ejecutandoConsumidor = true;
    try { consumidorDisplay(nullptr); } catch (const FinPrueba&) {}
    assert(Serial1.baud == 9600 && Serial1.config == SERIAL_8N1);
    assert(Serial1.tx == 13 && Serial1.rx == 14);
    assert(Serial1.bytes.size() >= 3);
    assert(Serial1.bytes[0] == 0x76 && Serial1.bytes[1] == 0x7A);
    assert(Serial1.bytes[2] == Config::BRILLO_DISPLAY);
    std::vector<std::string> pantallas;
    for (size_t i = 3; i < Serial1.bytes.size(); i += 6) {
        assert(i + 6 <= Serial1.bytes.size());
        assert(Serial1.bytes[i] == 0x79 && Serial1.bytes[i + 1] == 0);
        pantallas.emplace_back(Serial1.bytes.begin() + i + 2, Serial1.bytes.begin() + i + 6);
    }
    const std::vector<std::string> esperado = {
        "----", "  50", "  49", "1234", "  49", "----",
        "   0", " 100", "   0", "----", "  42"
    };
    assert(pantallas == esperado); // Sin reenvíos de números repetidos.
    Serial1.bytes.clear();
    DisplayCOM11441 driver(Serial1, 13, 14);
    driver.showText("1");
    assert(std::string(Serial1.bytes.begin() + 2, Serial1.bytes.end()) == "1   ");
    std::cout << "OK: Energía -> cola -> Display; SOC 50 %, UART TX13/RX14, "
                 "modo manual, sobrescritura, límites y ausencia de reenvíos.\n";
}
