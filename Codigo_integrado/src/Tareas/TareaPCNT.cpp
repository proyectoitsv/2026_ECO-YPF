#include "TareaPCNT.h"
#include <driver/gpio.h>
#include <driver/pcnt.h>
#include <esp_arduino_version.h>
#include <esp_err.h>
#include <soc/gpio_struct.h>
#include <soc/pcnt_struct.h>
#include "../Rtos/Rtos.h"
#include "../Rtos/Compartidos.h"

// Portado de "main PCNT.cpp" sin cambiar constantes, ISR ni buffer. La tensión (ahora en
// la tarea Energía) y la temperatura (tarea propia) ya no se leen aquí.
// ESP32-WROOM-32 | Arduino-ESP32 2.x | GPIO26 debe recibir una señal de 0-3,3 V.

#if ESP_ARDUINO_VERSION_MAJOR != 2
#error "Este archivo usa la API de timers de Arduino-ESP32 2.x."
#endif

#if !defined(CONFIG_IDF_TARGET_ESP32)
#error "La lectura directa de PCNT corresponde al ESP32 clasico/WROOM-32."
#endif

namespace {
// ---------------- CONFIGURACION MECANICA ----------------
constexpr int pinSensorInductivo = 26;
constexpr int pinBaseTiempo = 12;

constexpr float pulsosPorRevolucionMotor = 8.0f;
constexpr float diametroRuedaMetros = 0.49f;
constexpr uint64_t baseTiempoUs = 1000000ULL;
constexpr float baseTiempoSegundos = baseTiempoUs / 1000000.0f;
constexpr float circunferenciaRueda = PI * diametroRuedaMetros;

// Factores calculados en compilacion: loop() solo hace multiplicaciones.
constexpr float rpmPorPulso =
    60.0f / (pulsosPorRevolucionMotor * baseTiempoSegundos);
constexpr float kmhPorPulso =
     circunferenciaRueda * 3.6f /
    (pulsosPorRevolucionMotor * baseTiempoSegundos);

// ---------------- CONFIGURACION PCNT ----------------
constexpr pcnt_unit_t unidadPCNT = PCNT_UNIT_0;
constexpr int16_t limitePCNT = 30000;

// APB funciona a 80 MHz: 80 ciclos equivalen aproximadamente a 1 us.
// PCNT ignora los pulsos cuyo ancho sea menor o igual a este filtro.
// Usar 0 para desactivarlo si los pulsos validos son demasiado estrechos.
constexpr uint16_t filtroPCNT = 80;

// La resta solo puede compensar una vuelta a cero entre muestras.
// 30000 pulsos/s equivalen a 450000 rpm con 4 pulsos por vuelta.
static_assert(limitePCNT > 0, "El limite PCNT debe ser positivo");

// ---------------- COMUNICACION ISR -> LOOP ----------------
struct MuestraVelocidad {
    uint32_t numero;
    uint32_t pulsos;
    uint64_t totalPulsos;
};

// Dieciseis segundos de margen si loop() queda ocupado.
constexpr uint8_t capacidadBuffer = 16;
constexpr uint8_t mascaraBuffer = capacidadBuffer - 1;
static_assert((capacidadBuffer & mascaraBuffer) == 0,
              "La capacidad del buffer debe ser potencia de dos");

volatile MuestraVelocidad bufferMuestras[capacidadBuffer];
volatile uint8_t indiceEscritura = 0;
volatile uint8_t indiceLectura = 0;
volatile uint8_t cantidadMuestras = 0;
volatile uint32_t muestrasDescartadas = 0;

volatile uint64_t contadorPulsos = 0;
volatile uint32_t numeroMuestra = 0;

// Solo la ISR del timer las modifica una vez iniciada la medicion.
int16_t lecturaPCNTAnterior = 0;
bool estadoPinBaseTiempo = false;

hw_timer_t *timer = nullptr;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;



float rpm = 0.0f;
float velocidadKmH = 0.0f;

// Leer el registro evita llamar desde la ISR a una funcion que puede residir
// en Flash. Esta operacion es especifica del ESP32 clasico.
inline int16_t IRAM_ATTR leerPCNTDesdeISR() {
    return static_cast<int16_t>(PCNT.cnt_unit[unidadPCNT].cnt_val);
}

// ---------------- INTERRUPCION PERIODICA ----------------
void IRAM_ATTR onTimer() {
    const int16_t lecturaActual = leerPCNTDesdeISR();
    int32_t pulsosVentana =
        static_cast<int32_t>(lecturaActual) - lecturaPCNTAnterior;

    // Al alcanzar limitePCNT, el contador PCNT vuelve automaticamente a cero.
    // Ejemplo: anterior=29990 y actual=10 -> 20 pulsos nuevos.
    if (pulsosVentana < 0) {
        pulsosVentana += limitePCNT;
    }
    lecturaPCNTAnterior = lecturaActual;

    const uint32_t pulsosValidos = static_cast<uint32_t>(pulsosVentana);

    portENTER_CRITICAL_ISR(&mux);

    contadorPulsos += static_cast<uint64_t>(pulsosValidos);
    numeroMuestra++;

    // Si el buffer esta lleno, se descarta la muestra mas antigua. Esto
    // mantiene siempre disponibles los datos mas recientes y deja registro.
    if (cantidadMuestras == capacidadBuffer) {
        indiceLectura = (indiceLectura + 1U) & mascaraBuffer;
        muestrasDescartadas++;
    } else {
        cantidadMuestras++;
    }

    bufferMuestras[indiceEscritura].numero = numeroMuestra;
    bufferMuestras[indiceEscritura].pulsos = pulsosValidos;
    bufferMuestras[indiceEscritura].totalPulsos = contadorPulsos;
    indiceEscritura = (indiceEscritura + 1U) & mascaraBuffer;

    portEXIT_CRITICAL_ISR(&mux);

    // Salida de diagnostico: un cambio por segundo, periodo completo de 2 s.
    if (PCNT_SALIDA_DIAGNOSTICO) {
        estadoPinBaseTiempo = !estadoPinBaseTiempo;
        if (estadoPinBaseTiempo) {
            GPIO.out_w1ts = (1UL << pinBaseTiempo);
        } else {
            GPIO.out_w1tc = (1UL << pinBaseTiempo);
        }
    }
}

// ---------------- INICIALIZACION ----------------
void configurarPCNT() {
    pcnt_config_t config = {};
    config.pulse_gpio_num = pinSensorInductivo;
    config.ctrl_gpio_num = PCNT_PIN_NOT_USED;
    config.unit = unidadPCNT;
    config.channel = PCNT_CHANNEL_0;
    config.pos_mode = PCNT_COUNT_INC;
    config.neg_mode = PCNT_COUNT_DIS;
    config.lctrl_mode = PCNT_MODE_KEEP;
    config.hctrl_mode = PCNT_MODE_KEEP;
    config.counter_h_lim = limitePCNT;
    config.counter_l_lim = -limitePCNT;

    ESP_ERROR_CHECK(pcnt_unit_config(&config));
    ESP_ERROR_CHECK(pcnt_counter_pause(unidadPCNT));

    // La etapa de acondicionamiento externa debe definir ambos niveles.
    ESP_ERROR_CHECK(gpio_set_pull_mode(
        static_cast<gpio_num_t>(pinSensorInductivo), GPIO_FLOATING));

    if (filtroPCNT > 0) {
        ESP_ERROR_CHECK(pcnt_set_filter_value(unidadPCNT, filtroPCNT));
        ESP_ERROR_CHECK(pcnt_filter_enable(unidadPCNT));
    } else {
        ESP_ERROR_CHECK(pcnt_filter_disable(unidadPCNT));
    }

    // En el driver antiguo se habilita el evento de limite alto para asegurar
    // la vuelta controlada a cero. Su interrupcion permanece desactivada:
    // el unico instante de muestreo lo determina el timer.
    ESP_ERROR_CHECK(pcnt_event_enable(unidadPCNT, PCNT_EVT_H_LIM));
    ESP_ERROR_CHECK(pcnt_intr_disable(unidadPCNT));
}

void configurarTimer() {
    // Arduino-ESP32 2.x: APB/80 produce un tick por microsegundo.
    timer = timerBegin(0, 80, true);
    if (timer == nullptr) {
        logSerial("ERROR: no se pudo crear el timer.\n");
        vTaskSuspend(NULL);
    }

    timerStop(timer);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, baseTiempoUs, true);
    timerWrite(timer, 0);
    timerAlarmEnable(timer);
}

// Copia una muestra manteniendo el bloqueo solo durante unos pocos accesos.
bool extraerMuestra(MuestraVelocidad &muestra,
                    uint32_t &totalDescartadas) {
    bool disponible = false;

    portENTER_CRITICAL(&mux);
    if (cantidadMuestras > 0) {
        muestra.numero = bufferMuestras[indiceLectura].numero;
        muestra.pulsos = bufferMuestras[indiceLectura].pulsos;
        muestra.totalPulsos = bufferMuestras[indiceLectura].totalPulsos;
        indiceLectura = (indiceLectura + 1U) & mascaraBuffer;
        cantidadMuestras--;
        disponible = true;
    }
    totalDescartadas = muestrasDescartadas;
    portEXIT_CRITICAL(&mux);

    return disponible;
}

void procesarMuestra(const MuestraVelocidad &muestra) {
    // Los factores ya incluyen los 4 pulsos/vuelta y la ventana de 1 segundo.
    rpm = muestra.pulsos * rpmPorPulso;
    velocidadKmH = muestra.pulsos * kmhPorPulso;

    logSerial("Muestra: %lu | Pulsos/1 s: %lu | Total: %llu"
              " | RPM motor: %.2f | Velocidad: %.2f km/h\n",
              static_cast<unsigned long>(muestra.numero),
              static_cast<unsigned long>(muestra.pulsos),
              static_cast<unsigned long long>(muestra.totalPulsos),
              rpm, velocidadKmH);
}


void tarea(void*) {
    if (PCNT_SALIDA_DIAGNOSTICO) {
        pinMode(pinBaseTiempo, OUTPUT);
        digitalWrite(pinBaseTiempo, LOW);
    }

    // PCNT y timer se configuran en esta tarea: la ISR queda en el núcleo 1.
    configurarPCNT();
    configurarTimer();

    // Unico borrado por software. Desde aqui PCNT cuenta continuamente.
    ESP_ERROR_CHECK(pcnt_counter_clear(unidadPCNT));
    lecturaPCNTAnterior = 0;
    ESP_ERROR_CHECK(pcnt_counter_resume(unidadPCNT));
    timerStart(timer);

    logSerial("PCNT GPIO26: %.0f pulsos/vuelta y muestras cada 1 s.\n", pulsosPorRevolucionMotor);

    TickType_t ultimo = xTaskGetTickCount();
    uint32_t ultimoLatidoMs = millis();
    uint32_t descartadasInformadas = 0;
    for (;;) {
        MuestraVelocidad muestra = {};
        uint32_t totalDescartadas = 0;

        // Vaciar todas las muestras pendientes en orden cronologico.
        while (extraerMuestra(muestra, totalDescartadas)) {
            if (!leerPrimerPulso() && muestra.pulsos > 0) publicarPrimerPulso(true);
            procesarMuestra(muestra);
            publicarVelocidad({rpm, velocidadKmH, muestra.pulsos, muestra.totalPulsos});
        }

        if (totalDescartadas != descartadasInformadas) {
            logSerial("AVISO: buffer lleno; muestras descartadas: %lu\n",
                      static_cast<unsigned long>(totalDescartadas));
            descartadasInformadas = totalDescartadas;
        }

        if (HEARTBEAT_TAREAS && millis() - ultimoLatidoMs >= PERIODO_HEARTBEAT_MS) {
            ultimoLatidoMs = millis();
            logSerial("[PCNT] latido core=%d stack_libre=%u\n", xPortGetCoreID(),
                      (unsigned)uxTaskGetStackHighWaterMark(NULL));
        }
        vTaskDelayUntil(&ultimo, pdMS_TO_TICKS(PERIODO_PCNT_MS));
    }
}
} // namespace

bool crearTareaPCNT() {
    if (!HABILITAR_TAREA_PCNT) return true;
    return xTaskCreatePinnedToCore(tarea, "PCNT", STACK_PCNT, nullptr, PRIO_PCNT, nullptr, CORE_1) == pdPASS;
}
