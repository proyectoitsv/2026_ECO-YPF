#include "Reloj.h"
#include "ConfigRegistro.h"
#include <Arduino.h>
#include <freertos/queue.h>
namespace { SemaphoreHandle_t mutexHora; QueueHandle_t eventos; ReferenciaUTC referencia; uint32_t perdidos = 0; }
bool inicializarReloj() {
    referencia=ReferenciaUTC{}; perdidos=0;
    mutexHora=xSemaphoreCreateMutex(); eventos=xQueueCreate(16,sizeof(EventoHora));
    return mutexHora && eventos;
}
void sincronizarUTC(const FechaUTC& f, uint64_t us) {
    int64_t ms;
    if (!mutexHora || !convertirUnix(f,ms)) return;
    xSemaphoreTake(mutexHora,portMAX_DELAY);
    bool primera=!referencia.valida;
    const int64_t ajuste=aplicarSincronizacion(referencia,ms,us);
    const EventoHora e={us,referencia.sincronizaciones,ms,ajuste};
    if ((primera || ajuste > 250 || ajuste < -250) && xQueueSend(eventos,&e,0)!=pdPASS) ++perdidos;
    xSemaphoreGive(mutexHora);
}
HoraRegistro leerHora(uint64_t us) {
    ReferenciaUTC copia;
    if (!mutexHora) return {};
    xSemaphoreTake(mutexHora,portMAX_DELAY); copia=referencia; xSemaphoreGive(mutexHora);
    return estimarHora(copia,us,RegistroConfig::FRESCO_HORA_MS);
}
bool extraerEventoHora(EventoHora& e) { return eventos && xQueueReceive(eventos,&e,0)==pdTRUE; }
uint32_t eventosHoraPerdidos() {
    if (!mutexHora) return 0;
    xSemaphoreTake(mutexHora,portMAX_DELAY); uint32_t p=perdidos; xSemaphoreGive(mutexHora); return p;
}
