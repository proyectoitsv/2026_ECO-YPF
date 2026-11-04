#include <assert.h>
#include <vector>
#include <iostream>
#include "../src/Tareas/TareaTemperatura.cpp"
HardwareSerial Serial;
ESPFake ESP;
SemaphoreHandle_t mtxSerial=reinterpret_cast<void*>(1);
namespace FakeTemperatura {
bool parasita1=false;
std::vector<std::pair<unsigned,uint32_t>> solicitudes;
}
static uint32_t reloj=1000;
static std::vector<DatosTemperatura> lecturas;
struct Fin {};
uint32_t millis() { return reloj; }
int64_t esp_timer_get_time() { return int64_t(reloj)*1000; }
TickType_t xTaskGetTickCount() { return reloj; }
BaseType_t xSemaphoreTake(SemaphoreHandle_t,TickType_t) { return pdTRUE; }
BaseType_t xSemaphoreGive(SemaphoreHandle_t) { return pdTRUE; }
BaseType_t xTaskCreatePinnedToCore(void(*)(void*),const char*,uint32_t,void*,UBaseType_t,TaskHandle_t*,BaseType_t) { return pdPASS; }
void vTaskDelay(TickType_t t) { reloj+=t; }
void vTaskDelayUntil(TickType_t* anterior,TickType_t periodo) { *anterior+=periodo; assert(reloj<=*anterior); reloj=*anterior; }
void publicarTemperatura(const DatosTemperatura& t) { lecturas.push_back(t); if (lecturas.size()==3) throw Fin{}; }
int main() {
    try { tarea(nullptr); } catch (Fin&) {}
    assert(lecturas.size()==3 && FakeTemperatura::solicitudes.size()==6);
    for (unsigned i=0;i<3;++i) {
        assert(FakeTemperatura::solicitudes[2*i].second==1000+i*5000);
        assert(FakeTemperatura::solicitudes[2*i+1].second==1000+i*5000);
        assert(lecturas[i].captura1Us==(1750+i*5000)*1000ULL);
    }
    assert(lecturas[0].valida && lecturas[0].promedio==35);
    assert(lecturas[1].valida1 && !lecturas[1].valida2 && !lecturas[1].valida && isnan(lecturas[1].promedio));
    reloj=1000; lecturas.clear(); FakeTemperatura::solicitudes.clear(); FakeTemperatura::parasita1=true;
    try { tarea(nullptr); } catch (Fin&) {}
    assert(FakeTemperatura::solicitudes.size()==3);
    for (auto& s:FakeTemperatura::solicitudes) assert(s.first==33);
    assert(lecturas[0].parasitaria1 && !lecturas[0].valida1 && lecturas[0].valida2 && !lecturas[0].valida);
    std::cout<<"OK: tarea de temperatura real, conversiones paralelas 750 ms, periodo 5 s sin condicion de movimiento, desconexion individual y rechazo parasitario.\n";
}
