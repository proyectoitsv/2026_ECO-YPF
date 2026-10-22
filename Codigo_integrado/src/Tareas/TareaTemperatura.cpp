#include "TareaTemperatura.h"
#include "../Rtos/Rtos.h"
#include "../Rtos/Compartidos.h"
#include "../Sensores/Temperatura/Temperatura.h"
#include "../Registro/ConfigRegistro.h"
#include <esp_timer.h>
#include <math.h>
namespace {
void tarea(void*) {
    GestorTemperatura sensor1(25), sensor2(33);
    sensor1.inicializar(); sensor2.inicializar();
    // La decisión de alimentación se toma al arrancar; las dos conversiones
    // corren en paralelo y esta tarea duerme mientras esperan el resultado.
    const bool parasita1=sensor1.alimentacionParasitaria(), parasita2=sensor2.alimentacionParasitaria();
    if (parasita1 || parasita2) logSerial("Temperatura: alimentacion parasitaria; usar VDD externo de 3 hilos.\n");
    TickType_t inicio=xTaskGetTickCount();
    for (;;) {
        if (!parasita1) sensor1.solicitarTemperaturas();
        if (!parasita2) sensor2.solicitarTemperaturas();
        vTaskDelay(pdMS_TO_TICKS(750)); // Conversiones paralelas a 12 bits.
        DatosTemperatura t={};
        t.temp1=parasita1 ? -127.0f : sensor1.leerTemperatura(0); t.captura1Us=esp_timer_get_time();
        t.temp2=parasita2 ? -127.0f : sensor2.leerTemperatura(0); t.captura2Us=esp_timer_get_time();
        t.visto1=t.visto2=true; t.parasitaria1=parasita1; t.parasitaria2=parasita2;
        t.valida1=!parasita1 && isfinite(t.temp1) && t.temp1>=-55 && t.temp1<=125;
        t.valida2=!parasita2 && isfinite(t.temp2) && t.temp2>=-55 && t.temp2<=125;
        // El promedio exige dos lecturas buenas; cada temperatura individual
        // sigue disponible aunque la otra se desconecte.
        t.valida=t.valida1 && t.valida2; t.promedio=t.valida ? (t.temp1+t.temp2)/2.0f : NAN;
        publicarTemperatura(t);
        vTaskDelayUntil(&inicio,pdMS_TO_TICKS(RegistroConfig::TEMPERATURA_MS));
    }
}
}
bool crearTareaTemperatura() {
    if (!HABILITAR_TAREA_TEMPERATURA) return true;
    return xTaskCreatePinnedToCore(tarea,"Temperatura",STACK_TEMPERATURA,nullptr,PRIO_TEMPERATURA,nullptr,CORE_0)==pdPASS;
}
