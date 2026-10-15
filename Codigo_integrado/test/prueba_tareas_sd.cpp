#include <assert.h>
#include <deque>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "../src/Registro/Registro.cpp"
HardwareSerial Serial;
ESPFake ESP;
SDFake SD;
SemaphoreHandle_t mtxSerial=reinterpret_cast<void*>(1);
namespace FakeSD {
bool tarjeta=false, parcial=false;
unsigned montajes=0;
uint64_t libre=1024*1024;
std::map<std::string,std::string> archivos;
}
struct ColaFake { unsigned largo,tamano; std::deque<std::vector<uint8_t>> datos; };
static std::vector<std::unique_ptr<ColaFake>> colas;
static uint64_t relojFake=1000000;
static unsigned presupuesto;
static unsigned tareasCreadas;
static void (*alDormir)()=nullptr;
struct Fin {};
int64_t esp_timer_get_time() { return relojFake; }
uint32_t millis() { return relojFake/1000; }
TickType_t xTaskGetTickCount() { return millis(); }
SemaphoreHandle_t xSemaphoreCreateMutex() { return reinterpret_cast<void*>(1); }
BaseType_t xSemaphoreTake(SemaphoreHandle_t,TickType_t) { return pdTRUE; }
BaseType_t xSemaphoreGive(SemaphoreHandle_t) { return pdTRUE; }
BaseType_t xTaskCreatePinnedToCore(void(*)(void*),const char*,uint32_t,void*,UBaseType_t,TaskHandle_t*,BaseType_t) { ++tareasCreadas; return pdPASS; }
UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t) { return 2048; }
void vTaskDelay(TickType_t t) { relojFake+=uint64_t(t)*1000; if (alDormir) alDormir(); if (!--presupuesto) throw Fin{}; }
QueueHandle_t xQueueCreate(UBaseType_t largo,UBaseType_t tamano) {
    colas.emplace_back(new ColaFake{largo,tamano,{}}); return colas.back().get();
}
BaseType_t xQueueSend(QueueHandle_t q,const void* dato,TickType_t) {
    if (q->datos.size()==q->largo) return 0;
    auto p=static_cast<const uint8_t*>(dato); q->datos.emplace_back(p,p+q->tamano); return pdPASS;
}
BaseType_t xQueueReceive(QueueHandle_t q,void* dato,TickType_t espera) {
    if (q->datos.empty()) { if (espera==portMAX_DELAY) throw Fin{}; return 0; }
    memcpy(dato,q->datos.front().data(),q->tamano); q->datos.pop_front(); return pdTRUE;
}
UBaseType_t uxQueueMessagesWaiting(QueueHandle_t q) { return q->datos.size(); }
DatosCompartidos copiarDatosCompartidos() { DatosCompartidos d={}; d.energia.ahConsumidos=7.5f; return d; }
SnapshotRegistro capturarSnapshotRegistro() { SnapshotRegistro s={}; s.capturaUs=relojFake; s.datos=copiarDatosCompartidos(); return s; }
void ejecutar(void(*f)(void*),unsigned vueltas) { presupuesto=vueltas; try { f(nullptr); } catch (Fin&) {} }
void preparar() {
    control=Control{}; relojFake+=1000000; FakeSD::montajes=0; FakeSD::archivos.clear(); FakeSD::parcial=false;
    FakeSD::libre=1024*1024;
    Serial.salida.clear(); assert(prepararRegistro()); comenzarSesionRegistro();
}
int main(int argc,char** argv) {
    preparar(); assert(crearTareasRegistro() && tareasCreadas==3);
    ejecutar(tareaCaptura,4001);
    assert(control.secuencia==41 && control.perdidos==9 && uxQueueMessagesWaiting(colaSD)==32);
    assert(FakeSD::montajes==0); // La captura real nunca toca SD/SPI.
    RegistroFila primero,ultimo;
    memcpy(&primero,colaSD->datos.front().data(),sizeof(primero));
    memcpy(&ultimo,colaSD->datos.back().data(),sizeof(ultimo));
    assert(primero.timestampMs==0 && ultimo.timestampMs==31000);
    FakeSD::tarjeta=false; ejecutar(tareaSD,501);
    assert(uxQueueMessagesWaiting(colaSD)==32 && FakeSD::montajes==2);
    FakeSD::tarjeta=true; ejecutar(tareaSD,40);
    assert(control.escritos==32 && uxQueueMessagesWaiting(colaSD)==0 && control.perdidos==9);
    unsigned csv=0;
    for (auto& e:FakeSD::archivos) if (e.first.find(".csv")!=std::string::npos) {
        ++csv; assert(std::count(e.second.begin(),e.second.end(),'\n')==33);
        assert(e.second.find(",31000,")!=std::string::npos);
    }
    assert(csv==1);

    preparar(); FakeSD::tarjeta=false; ejecutar(tareaCaptura,101);
    assert(procesarComandoRegistro("sd cerrar")); ejecutar(tareaCaptura,1);
    ejecutar(tareaSD,2); assert(control.cerrado && !control.capturando && control.perdidos==2);
    const uint64_t t0=control.t0; std::string id=control.id;
    assert(procesarComandoRegistro("sd nueva")); ejecutar(tareaCaptura,1);
    assert(control.capturando && control.t0>t0 && id!=control.id && control.inicial.ahConsumidos==7.5f);

    preparar(); FakeSD::tarjeta=true; ejecutar(tareaCaptura,301);
    FakeSD::parcial=true; ejecutar(tareaSD,520);
    assert(control.perdidos==1 && control.escritos==3 && uxQueueMessagesWaiting(colaSD)==0);
    csv=0; unsigned completos=0;
    for (auto& e:FakeSD::archivos) if (e.first.find(".csv")!=std::string::npos) {
        ++csv; if (!e.second.empty() && e.second.back()=='\n') ++completos;
    }
    assert(csv==2 && completos==1); // Una parte incierta preservada, otra recuperada.
    preparar(); FakeSD::tarjeta=true; ejecutar(tareaCaptura,101);
    alDormir=+[]() { if (presupuesto==8) sincronizarUTC({2026,10,14,17,0,35,0},relojFake); };
    ejecutar(tareaSD,10); alDormir=nullptr;
    bool conFecha=false, conEnlace=false;
    for (auto& e:FakeSD::archivos) {
        if (e.first.find("Prueba_14-10-2026")!=std::string::npos) conFecha=true;
        if (e.first.find("Meta_")!=std::string::npos && e.second.find("SYNC numero=")!=std::string::npos) conEnlace=true;
    }
    assert(conFecha && conEnlace && leerHora(relojFake).fuente==FuenteHora::GPS);
    // Tarjeta llena: no crear archivos vacíos ni consumir la cola antes de recuperar espacio.
    preparar(); FakeSD::tarjeta=true; ejecutar(tareaCaptura,201);
    FakeSD::libre=0; ejecutar(tareaSD,501);
    assert(control.escritos==0 && uxQueueMessagesWaiting(colaSD)==3 && FakeSD::archivos.empty());
    FakeSD::libre=1024*1024;
    // Entrega del estado de cierre mientras la misma instancia escritora conserva sus handles.
    alDormir=+[]() { if (control.escritos==3) { control.capturando=false; control.cerrar=true; } };
    ejecutar(tareaSD,501); alDormir=nullptr;
    assert(control.escritos==3 && control.perdidos==0);
    assert(control.cerrado && control.escritos==3 && control.perdidos==0);
    for (const auto& e:FakeSD::archivos) if (e.first.find("Meta_")!=std::string::npos) {
        assert(e.second.find("firmware_base=e4ca276507141481e6e8dc57f8706e6747a8eb69")!=std::string::npos);
        assert(e.second.find("CIERRE mono_us=")!=std::string::npos);
        if (argc>1) { std::ofstream meta(argv[1]); meta<<e.second; }
        std::cout<<"metadatos_simulados_con_cierre_bytes="<<e.second.size()<<"\n";
    }
    preparar(); FakeSD::tarjeta=true; ejecutar(tareaCaptura,4001);
    alDormir=+[]() {
        if (control.escritos==1) FakeSD::libre=0;
        if (presupuesto==100) { control.capturando=false; control.cerrar=true; }
    };
    ejecutar(tareaSD,150); alDormir=nullptr;
    assert(control.cerrado && control.escritos==1 && control.perdidos==40);
    csv=0;
    for (const auto& e:FakeSD::archivos) if (e.first.find(".csv")!=std::string::npos) {
        ++csv; assert(std::count(e.second.begin(),e.second.end(),'\n')==2);
    }
    assert(csv==1); // Conserva la parte abierta al llenarse; no genera partes vacías.
    std::cout<<"OK: tareas reales de captura/SD, cola llena descarta nuevo, ausencia/reintento, timestamps diferidos, cierre, nueva sesion y escritura parcial con recuperacion.\n";
}
