#include "Registro.h"
#include "ConfigRegistro.h"
#include "CSV.h"
#include "Panel.h"
#include "Archivo.h"
#include "Reloj.h"
#include "../Rtos/Rtos.h"
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Preferences.h>
#include <esp_timer.h>
#include <esp_system.h>
#include <esp_arduino_version.h>
#include <sdkconfig.h>
#include <freertos/queue.h>
#include <string.h>
#define ECO_STR_I(x) #x
#define ECO_STR(x) ECO_STR_I(x)
#define ECO_CORE_VERSION ECO_STR(ESP_ARDUINO_VERSION_MAJOR) "." ECO_STR(ESP_ARDUINO_VERSION_MINOR) "." ECO_STR(ESP_ARDUINO_VERSION_PATCH)

namespace {
using namespace RegistroConfig;
enum class Orden : uint8_t { ESTADO, CERRAR, NUEVA };
struct Control {
    // Contadores y última situación administrativa; las lecturas físicas viven
    // en el snapshot, no en esta estructura.
    bool capturando=false, cerrar=false, cerrado=false, pedirEstado=false;
    // Estos indicadores reflejan si pudo arrancar cada tarea del registrador.
    bool capturaDisponible=true, sdDisponible=true, monitorDisponible=true, metaPendiente=false;
    uint64_t t0=0, secuencia=0, perdidos=0, escritos=0, omitidos=0;
    uint32_t fallosMeta=0;
    uint32_t sesion=0, maxCola=0, maxEscrituraUs=0, maxSyncUs=0;
    uint32_t stackCaptura=0, stackSD=0, stackMonitor=0;
    char id[56]={}, ruta[160]={}, error[96]={};
    EstadoEnergia inicial={};
};
Control control;
SemaphoreHandle_t mutexControl;
QueueHandle_t colaSD, colaMonitor, comandos;
uint64_t bootId;
bool bootPersistente=false;
bool registroPreparado=false;
SPIClass bus(VSPI);
File fichero;
#if defined(CONFIG_FATFS_LFN_NONE) && CONFIG_FATFS_LFN_NONE
constexpr bool LARGOS=false;
#else
constexpr bool LARGOS=true;
#endif
Control leerControl() {
    // Copia breve para no mantener el candado mientras se imprime o usa SD.
    xSemaphoreTake(mutexControl,portMAX_DELAY); Control c=control; xSemaphoreGive(mutexControl); return c;
}
void perder(uint64_t cantidad=1) {
    xSemaphoreTake(mutexControl,portMAX_DELAY); control.perdidos+=cantidad; xSemaphoreGive(mutexControl);
}
void errorSD(const char* texto) {
    xSemaphoreTake(mutexControl,portMAX_DELAY); snprintf(control.error,sizeof(control.error),"%s",texto); xSemaphoreGive(mutexControl);
    logSerial("[SD] %s\n",texto);
}
bool existe(void*,const char* p) { return SD.exists(p); }
bool abrir(void*,const char* p, bool append) { fichero=SD.open(p,append ? FILE_APPEND : FILE_WRITE); return bool(fichero); }
size_t escribir(void*,const char* s,size_t n) { return fichero.write(reinterpret_cast<const uint8_t*>(s),n); }
void sync(void*) { fichero.flush(); }
bool errorArchivo(void*) { return fichero.getWriteError()!=0; }
void cerrar(void*) { fichero.close(); }
bool renombrar(void*,const char* a,const char* b) { return SD.rename(a,b); }
const BackendArchivo backend={nullptr,existe,abrir,escribir,sync,errorArchivo,cerrar,renombrar};

void nuevaSesion() {
    const DatosCompartidos d=copiarDatosCompartidos();
    // Descarta exclusivamente la ventana auxiliar anterior, no los Ah/Wh.
    capturarSnapshotRegistro();
    // Inicia otro reloj relativo, sin reiniciar la energía ni el SOC.
    const uint64_t t0=esp_timer_get_time();
    xSemaphoreTake(mutexControl,portMAX_DELAY);
    ++control.sesion; control.t0=t0; control.secuencia=control.perdidos=control.escritos=control.omitidos=0;
    control.maxCola=0; control.cerrar=false; control.cerrado=false;
    control.capturando=control.capturaDisponible && control.sdDisponible;
    control.metaPendiente=false; control.fallosMeta=0;
    control.ruta[0]=control.error[0]=0; control.inicial=d.energia;
    snprintf(control.id,sizeof(control.id),"%012llX-%016llX-%04lu",
        static_cast<unsigned long long>(ESP.getEfuseMac()),static_cast<unsigned long long>(bootId),static_cast<unsigned long>(control.sesion));
    xSemaphoreGive(mutexControl);
}
void capturar() {
    // Primero fijamos la referencia del reloj. Si el GPS la actualiza mientras
    // copiamos los sensores, esta fila sigue usando una referencia anterior a ella.
    const ReferenciaUTC horaBase=copiarReferenciaHora();
    RegistroFila f={}; f.snapshot=capturarSnapshotRegistro();
    xSemaphoreTake(mutexControl,portMAX_DELAY);
    f.timestampMs=(f.snapshot.capturaUs-control.t0)/1000;
    f.registro=control.secuencia++; f.perdidos=control.perdidos; f.omitidosMonitor=control.omitidos;
    const bool monitorDisponible=control.monitorDisponible;
    strcpy(f.sessionId,control.id); xSemaphoreGive(mutexControl);
    f.hora=estimarHora(horaBase,f.snapshot.capturaUs,FRESCO_HORA_MS);
    // La captura no espera a la tarjeta: si se llena la cola se pierde esta
    // fila nueva, avanza su número y se cuenta la pérdida conocida.
    if (xQueueSend(colaSD,&f,0)!=pdPASS) perder();
    const uint32_t usados=uxQueueMessagesWaiting(colaSD);
    xSemaphoreTake(mutexControl,portMAX_DELAY);
    if (usados>control.maxCola) control.maxCola=usados;
    xSemaphoreGive(mutexControl);
    if (MONITOR_CSV && monitorDisponible && xQueueSend(colaMonitor,&f,0)!=pdPASS) {
        xSemaphoreTake(mutexControl,portMAX_DELAY); ++control.omitidos; xSemaphoreGive(mutexControl);
    }
}
void tareaCaptura(void*) {
    bool nuevaPendiente=false;
    uint64_t siguiente=leerControl().t0;
    for (;;) {
        Orden o;
        while (xQueueReceive(comandos,&o,0)==pdTRUE) {
            // Solo esta tarea cambia el ciclo de vida de las sesiones; la
            // escritora termina lo pendiente antes de iniciar una nueva.
            xSemaphoreTake(mutexControl,portMAX_DELAY);
            if (o==Orden::ESTADO) control.pedirEstado=true;
            else { control.capturando=false; control.cerrar=true; nuevaPendiente=o==Orden::NUEVA; }
            xSemaphoreGive(mutexControl);
        }
        Control c=leerControl();
        if (nuevaPendiente && c.cerrado && c.sdDisponible && c.capturaDisponible) {
            nuevaSesion(); nuevaPendiente=false; c=leerControl(); siguiente=c.t0;
        }
        const uint64_t ahora=esp_timer_get_time();
        if (c.capturando && ahora>=siguiente) {
            // Una sola captura actual. No se fabrican filas de segundos perdidos.
            const uint64_t saltados=(ahora-siguiente)/(uint64_t(CAPTURA_MS)*1000);
            if (saltados) {
                xSemaphoreTake(mutexControl,portMAX_DELAY); control.secuencia+=saltados; control.perdidos+=saltados; xSemaphoreGive(mutexControl);
            }
            capturar(); siguiente+=(saltados+1)*uint64_t(CAPTURA_MS)*1000;
        }
        xSemaphoreTake(mutexControl,portMAX_DELAY); control.stackCaptura=uxTaskGetStackHighWaterMark(nullptr); xSemaphoreGive(mutexControl);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
bool escribirMeta(File& meta, const char* texto) {
    const size_t n=strlen(texto);
    return meta && meta.write(reinterpret_cast<const uint8_t*>(texto),n)==n && !meta.getWriteError();
}
void falloMetadatos(File& meta, bool& pendiente, uint64_t& proximo, uint64_t ahora,
                    const char* mensaje) {
    // La falla del acompañante nunca cierra el CSV de mediciones.
    if (meta) meta.close();
    pendiente=true; proximo=ahora+uint64_t(REINTENTO_MS)*1000;
    xSemaphoreTake(mutexControl,portMAX_DELAY);
    control.metaPendiente=true; ++control.fallosMeta;
    xSemaphoreGive(mutexControl);
    errorSD(mensaje);
}
bool espacioPara(size_t n) {
    // Reserva además un margen para evitar iniciar una escritura sin espacio.
    const uint64_t total=SD.totalBytes(), usado=SD.usedBytes();
    return total>usado && total-usado>=n+4096;
}
void tareaSD(void*) {
    ArchivoRegistro a;
    File meta;
    bool montada=false, fechado=false, metaPendiente=false, metaCabeceraLista=false;
    bool cierreMetaIntentado=false;
    uint32_t sesionVista=0, parte=0;
    uint64_t proximoIntento=0, proximaEscritura=0, ultimoSync=0, proximoMeta=0;
    char fila[MAX_CSV], info[2400], rutaMeta[100]={};
    // Se conserva solo el encabezado de la parte actual para poder reintentarlo
    // si la tarjeta rechazó inicialmente el archivo de metadatos.
    static char cabeceraMeta[2400];
    for (;;) {
        Control c=leerControl();
        // También el nombre del archivo se calcula con una referencia tomada
        // antes de la hora que se quiere convertir.
        const ReferenciaUTC horaBase=copiarReferenciaHora();
        const uint64_t ahora=esp_timer_get_time();
        if (c.sesion!=sesionVista) {
            sesionVista=c.sesion; parte=0; fechado=false; a=ArchivoRegistro{};
            rutaMeta[0]=cabeceraMeta[0]=0; metaPendiente=metaCabeceraLista=false;
            cierreMetaIntentado=false;
            proximoIntento=proximaEscritura=proximoMeta=0;
        }
        if (!a.abierto && !c.cerrado && (c.capturando || uxQueueMessagesWaiting(colaSD)) && ahora>=proximoIntento) {
            proximoIntento=ahora+uint64_t(REINTENTO_MS)*1000;
            if (montada) { if (meta) meta.close(); SD.end(); montada=false; }
            pinMode(RegistroConfig::CS,OUTPUT); digitalWrite(RegistroConfig::CS,HIGH);
            bus.begin(RegistroConfig::SCK,RegistroConfig::MISO,RegistroConfig::MOSI,RegistroConfig::CS);
            montada=SD.begin(RegistroConfig::CS,bus,SPI_HZ,"/sd",5,false);
            if (!montada || SD.cardType()==CARD_NONE) errorSD("SIN_TARJETA_O_MONTAJE; reintento espaciado");
            else {
                const HoraRegistro h=estimarHora(horaBase,ahora,FRESCO_HORA_MS);
                const int64_t inicio=h.fuente==FuenteHora::SIN_HORA ? 0 : h.unixMs-static_cast<int64_t>((ahora-c.t0)/1000);
                if (!espacioPara(MAX_CSV+strlen(cabeceraCSV()))) errorSD("TARJETA_LLENA_O_CAPACIDAD_INVALIDA; cola conservada hasta su limite");
                else if (!crearArchivoRegistro(a,backend,inicio,parte++,LARGOS,cabeceraCSV())) errorSD("CREACION_O_CABECERA_FALLIDA; archivo previo preservado");
                else {
                    fechado=inicio>0; ultimoSync=ahora;
                    // Sidecar estable por session_id. En 8.3 se usa el nombre del CSV.
                    if (LARGOS) snprintf(rutaMeta,sizeof(rutaMeta),"/Meta_%s.txt",c.id);
                    else { snprintf(rutaMeta,sizeof(rutaMeta),"%s",a.ruta); char* ext=strrchr(rutaMeta,'.'); if (ext) strcpy(ext,".TXT"); }
                    meta=SD.open(rutaMeta,FILE_APPEND);
                    int n=snprintf(info,sizeof(info),
                        "session_id=%s\nschema_version=2\nfirmware_base=%s\nversion=%s\narduino=%s\nreset_reason=%d\n"
                        "t0_us=%llu\nboot_persistente=%d\nUTC_offset=-03:00\nPPS=no\ninicio_absoluto_estimado=%d\n"
                        "inicio_unix_ms_estimado=%lld\nreferencia_mono_us=%llu\nreferencia_unix_ms=%lld\n"
                        "csv=%s\nparte=%lu\nLFN=%d\ncola_items=%u\nsizeof_fila=%u\nSPI_Hz=%lu\ncaptura_ms=%lu\nsync_ms=%lu\n"
                        "modo_energia=%s\ncapacidad_Ah=%.4f\ntension_nominal_V=%.2f\npin_tension=%u\npin_corriente=%u\n"
                        "escala_ADC_V=%.3f,%.3f\nescala_pack_V=%.2f,%.2f\nsensibilidad_V_A=%.6f\noffset_config_V=%.6f\nfactor_corriente=%.6f\nsigno_corriente=%.0f\n"
                        "I_muestreo_ms=%lu\nI_muestras=%u\nV_muestreo_ms=%lu\nV_muestras=%u\nenergia_periodo_ms=20\nPCNT_pulsos_vuelta=8\nPCNT_ventana_ms=1000\n"
                        "temperatura_periodo_ms=5000\nDS18B20_bits=12\nGPS_posicion_Hz_objetivo=1\nGPS_UART_baudios=9600\n"
                        "ADC_bits=%u\ncalibracion_corriente_modo=%u\ncalibracion_muestras=%u\ncalibracion_intervalo_ms=%lu\n"
                        "pin_SD_CS=5\npin_SD_SCK=18\npin_SD_MISO=19\npin_SD_MOSI=23\npin_PCNT=26\npin_temp1=25\npin_temp2=33\npin_GPS_RX=16\npin_GPS_TX=17\n"
                        "inicial_Ah_consumidos=%.6f\ninicial_Wh_consumidos=%.6f\n"
                        "estados=OK,ANTIGUA,SIN_DATO,INVALIDA,CALIBRANDO,NO_DISPONIBLE,DESCONECTADO,ERROR_ALIMENTACION,SIN_FIX,SIN_RUMBO,SIMULADA\n"
                        "fuentes_hora=GPS,ESTIMADA,SIN_HORA\nflush=API_void_no_garantiza_persistencia\n",
                        c.id,BASE_COMMIT,VERSION,ECO_CORE_VERSION,int(esp_reset_reason()),
                        static_cast<unsigned long long>(c.t0),bootPersistente,inicio>0,static_cast<long long>(inicio),static_cast<unsigned long long>(ahora),static_cast<long long>(h.unixMs),a.ruta,static_cast<unsigned long>(parte-1),LARGOS,COLA,unsigned(sizeof(RegistroFila)),
                        static_cast<unsigned long>(SPI_HZ),static_cast<unsigned long>(CAPTURA_MS),static_cast<unsigned long>(SYNC_MS),nombreModoRegistro(uint8_t(Config::MODO)),
                        Config::CAPACIDAD_PACK_AH,Config::TENSION_NOMINAL_PACK_V,Config::PIN_TENSION,Config::PIN_CORRIENTE,
                        Config::ENTRADA_TENSION_MIN_V,Config::ENTRADA_TENSION_MAX_V,Config::PACK_MIN_V,Config::PACK_MAX_V,
                        Config::SENSIBILIDAD_CORRIENTE_V_POR_A,Config::OFFSET_CORRIENTE_V,Config::FACTOR_SALIDA_SENSOR_SOBRE_ADC,Config::SIGNO_CORRIENTE,
                        static_cast<unsigned long>(Config::INTERVALO_CORRIENTE_MS),Config::MUESTRAS_CORRIENTE,static_cast<unsigned long>(Config::INTERVALO_TENSION_MS),Config::MUESTRAS_TENSION,
                        Config::RESOLUCION_ADC_BITS,unsigned(Config::CALIBRACION_CORRIENTE),Config::MUESTRAS_CALIBRACION,static_cast<unsigned long>(Config::INTERVALO_CALIBRACION_MS),
                        c.inicial.ahConsumidos,c.inicial.whConsumidos);
                    if (n<0 || size_t(n)>=sizeof(info)) {
                        // Un encabezado mayor que el buffer no puede recuperarse reintentando.
                        if (meta) meta.close();
                        errorSD("METADATOS_EXCEDEN_BUFFER; CSV conservado");
                    } else {
                        memcpy(cabeceraMeta,info,size_t(n)+1);
                        metaCabeceraLista=escribirMeta(meta,info);
                        if (!metaCabeceraLista)
                            falloMetadatos(meta,metaPendiente,proximoMeta,ahora,"METADATOS_FALLIDOS; CSV conservado");
                    }
                    xSemaphoreTake(mutexControl,portMAX_DELAY); snprintf(control.ruta,sizeof(control.ruta),"%s",a.ruta); xSemaphoreGive(mutexControl);
                    logSerial("[SD] archivo %s session_id=%s\n",a.ruta,c.id);
                }
            }
        }
        if (a.abierto && metaPendiente && rutaMeta[0] && cabeceraMeta[0] &&
            (ahora>=proximoMeta || (c.cerrar && !cierreMetaIntentado))) {
            if (c.cerrar) cierreMetaIntentado=true; // Un intento urgente, no uno cada 20 ms.
            // Reabrir solo el acompañante. FILE_WRITE corrige un encabezado
            // inicial incompleto; FILE_APPEND respeta metadatos ya guardados.
            meta=SD.open(rutaMeta,metaCabeceraLista ? FILE_APPEND : FILE_WRITE);
            bool recuperada=bool(meta);
            if (recuperada && !metaCabeceraLista) {
                recuperada=escribirMeta(meta,cabeceraMeta);
                if (recuperada) metaCabeceraLista=true;
            }
            if (recuperada) {
                snprintf(info,sizeof(info),"RECUPERACION_META mono_us=%llu csv_actual=%s eventos_previos_no_garantizados=1\n",
                         static_cast<unsigned long long>(ahora),a.ruta);
                recuperada=escribirMeta(meta,info);
            }
            if (recuperada) {
                metaPendiente=false;
                xSemaphoreTake(mutexControl,portMAX_DELAY); control.metaPendiente=false; xSemaphoreGive(mutexControl);
            } else falloMetadatos(meta,metaPendiente,proximoMeta,ahora,
                                  "REINTENTO_META_FALLIDO; CSV conservado");
        }
        if (a.abierto) {
            const HoraRegistro h=estimarHora(horaBase,ahora,FRESCO_HORA_MS);
            if (!fechado && h.fuente!=FuenteHora::SIN_HORA && LARGOS) {
                const int64_t inicio=h.unixMs-static_cast<int64_t>((ahora-c.t0)/1000);
                const bool ok=fecharArchivo(a,backend,inicio,parte-1,LARGOS);
                // Si falla rename y se reabre el original, se conserva su ruta.
                if (!ok) errorSD("RENOMBRE_FALLIDO; ruta original conservada si pudo reabrirse");
                else {
                    fechado=true;
                    snprintf(info,sizeof(info),"RENOMBRE csv=%s inicio_unix_estimado=%lld\n",a.ruta,static_cast<long long>(inicio));
                    if (meta && !escribirMeta(meta,info))
                        falloMetadatos(meta,metaPendiente,proximoMeta,ahora,"EVENTO_RENOMBRE_META_FALLIDO");
                    // Si falta el archivo, la marca de recuperación registrará
                    // el nombre actual; no se pierde ni se renombra el CSV otra vez.
                }
                xSemaphoreTake(mutexControl,portMAX_DELAY); snprintf(control.ruta,sizeof(control.ruta),"%s",a.ruta); xSemaphoreGive(mutexControl);
                if (!ok) fechado=true; // No reintentar rename continuamente; el enlace UTC queda en metadatos.
            }
            EventoHora e;
            while (meta && extraerEventoHora(e)) {
                snprintf(info,sizeof(info),"SYNC numero=%llu mono_us=%llu unix_ms=%lld ajuste_ms=%lld\n",static_cast<unsigned long long>(e.numero),static_cast<unsigned long long>(e.monoUs),static_cast<long long>(e.unixMs),static_cast<long long>(e.ajusteMs));
                if (!escribirMeta(meta,info)) {
                    falloMetadatos(meta,metaPendiente,proximoMeta,ahora,"EVENTO_SYNC_META_FALLIDO");
                    break; // Los eventos siguientes permanecen en la cola.
                }
            }
            RegistroFila f;
            // Una fila por iteración. SPI y archivos pertenecen solo a esta
            // tarea: el mutex de sensores ya se soltó antes de llegar aquí.
            if (a.abierto && (ahora>=proximaEscritura || c.cerrar) && xQueueReceive(colaSD,&f,0)==pdTRUE) {
                const size_t n=formatearCSV(f,fila,sizeof(fila));
                const uint64_t t=esp_timer_get_time();
                if (!n) { perder(); errorSD("FILA_EXCEDE_BUFFER; descartada completa"); }
                else if (!espacioPara(n)) {
                    // No se intentó escribir: el archivo actual sigue íntegro y abierto.
                    perder(); proximaEscritura=esp_timer_get_time()+uint64_t(REINTENTO_MS)*1000;
                    errorSD("TARJETA_LLENA; conserva archivo y cola, reintenta en 10 s");
                } else if (!escribirFilaArchivo(a,backend,fila,n)) {
                    // Otra parte conserva las filas completas anteriores;
                    // nunca se agrega una fila a un fragmento posiblemente roto.
                    perder(); a.incierto=true; if (a.abierto) { fichero.close(); a.abierto=false; }
                    proximoIntento=esp_timer_get_time()+uint64_t(REINTENTO_MS)*1000;
                    errorSD("ESCRITURA_PARCIAL; nueva parte de recuperacion en reintento");
                } else { xSemaphoreTake(mutexControl,portMAX_DELAY); ++control.escritos; xSemaphoreGive(mutexControl); }
                const uint32_t dur=esp_timer_get_time()-t;
                xSemaphoreTake(mutexControl,portMAX_DELAY); if (dur>control.maxEscrituraUs) control.maxEscrituraUs=dur; xSemaphoreGive(mutexControl);
            }
            if (a.abierto && ahora-ultimoSync>=uint64_t(SYNC_MS)*1000) {
                const uint64_t t=esp_timer_get_time();
                if (!sincronizarArchivo(a,backend)) { fichero.close(); a.abierto=false; proximoIntento=esp_timer_get_time()+uint64_t(REINTENTO_MS)*1000; errorSD("ERROR_SYNC; parte incierta preservada"); }
                if (meta) {
                    meta.flush();
                    if (meta.getWriteError())
                        falloMetadatos(meta,metaPendiente,proximoMeta,ahora,"SYNC_META_FALLIDO");
                }
                ultimoSync=ahora;
                const uint32_t dur=esp_timer_get_time()-t;
                xSemaphoreTake(mutexControl,portMAX_DELAY); if (dur>control.maxSyncUs) control.maxSyncUs=dur; xSemaphoreGive(mutexControl);
            }
        }
        if (c.cerrar && !c.cerrado) {
            if (!a.abierto) {
                RegistroFila f;
                while (xQueueReceive(colaSD,&f,0)==pdTRUE) perder();
                errorSD("CIERRE_SIN_ARCHIVO: pendientes descartados y contabilizados");
            }
            if (!uxQueueMessagesWaiting(colaSD)) {
                if (a.abierto && !cerrarArchivo(a,backend)) errorSD("CIERRE_CON_ERROR_SYNC");
                if (meta) {
                    const Control final=leerControl();
                    snprintf(info,sizeof(info),"CIERRE mono_us=%llu escritos=%llu perdidos=%llu monitor_omitidos=%llu eventos_hora_perdidos=%lu\n",
                        static_cast<unsigned long long>(esp_timer_get_time()),static_cast<unsigned long long>(final.escritos),
                        static_cast<unsigned long long>(final.perdidos),static_cast<unsigned long long>(final.omitidos),static_cast<unsigned long>(eventosHoraPerdidos()));
                    if (!escribirMeta(meta,info)) errorSD("EVENTO_CIERRE_META_FALLIDO");
                    meta.flush(); meta.close();
                }
                xSemaphoreTake(mutexControl,portMAX_DELAY); control.cerrado=true; xSemaphoreGive(mutexControl);
                logSerial("[SD] cerrado; metadatos_pendientes=%u; flush solicitado, persistencia fisica no verificable por API.\n",metaPendiente);
            }
        }
        xSemaphoreTake(mutexControl,portMAX_DELAY);
        bool informar=control.pedirEstado; control.pedirEstado=false; control.stackSD=uxTaskGetStackHighWaterMark(nullptr);
        xSemaphoreGive(mutexControl);
        if (informar) {
            c=leerControl();
            logSerial("[SD] activa=%u cerrada=%u escritos=%llu perdidos=%llu cola=%u/%u max=%lu\n",c.capturando,c.cerrado,static_cast<unsigned long long>(c.escritos),static_cast<unsigned long long>(c.perdidos),unsigned(uxQueueMessagesWaiting(colaSD)),COLA,static_cast<unsigned long>(c.maxCola));
            logSerial("[SD] archivo=%s abierto=%u montada=%u\n",c.ruta,a.abierto,montada);
            logSerial("[SD] heap_libre=%lu bytes fila=%u cola_RAM=%u bytes\n",static_cast<unsigned long>(ESP.getFreeHeap()),unsigned(sizeof(RegistroFila)),unsigned(sizeof(RegistroFila)*COLA));
            logSerial("[SD] ultimo_error_registrado=%s serial_omitidos=%llu eventos_hora_perdidos_cola=%lu fallos_meta=%lu meta_pendiente=%u\n",c.error,static_cast<unsigned long long>(c.omitidos),static_cast<unsigned long>(eventosHoraPerdidos()),static_cast<unsigned long>(c.fallosMeta),c.metaPendiente);
            logSerial("[SD] tareas captura=%u escritora=%u monitor=%u\n",c.capturaDisponible,c.sdDisponible,c.monitorDisponible);
            logSerial("[SD] max_write_us=%lu max_sync_us=%lu stack_libre captura/SD/monitor=%lu/%lu/%lu\n",static_cast<unsigned long>(c.maxEscrituraUs),static_cast<unsigned long>(c.maxSyncUs),static_cast<unsigned long>(c.stackCaptura),static_cast<unsigned long>(c.stackSD),static_cast<unsigned long>(c.stackMonitor));
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
void tareaMonitor(void*) {
    RegistroFila f;
    // Un solo buffer fijo de esta tarea evita grandes arreglos en su stack.
    static char panel[1600];
    for (;;) {
        if (xQueueReceive(colaMonitor,&f,portMAX_DELAY)!=pdTRUE) continue;
        const Control c=leerControl();
        const EstadoPanelRegistro estado={c.capturando,c.cerrado,c.sdDisponible,c.monitorDisponible,
                                          c.metaPendiente,c.escritos,c.perdidos,c.omitidos,c.error};
        const size_t n=formatearPanelRegistro(f,estado,panel,sizeof(panel));
        bool emitida=false;
        if (n && xSemaphoreTake(mtxSerial,0)==pdTRUE) {
            // Un único envío solo si cabe entero en el espacio anunciado por UART.
            // Si está ocupado, se omite este panel; la fila SD ya está en otra cola.
            if (Serial.availableForWrite()>=int(n)) emitida=Serial.write(reinterpret_cast<const uint8_t*>(panel),n)==n;
            xSemaphoreGive(mtxSerial);
        }
        xSemaphoreTake(mutexControl,portMAX_DELAY);
        if (!emitida) ++control.omitidos;
        control.stackMonitor=uxTaskGetStackHighWaterMark(nullptr); xSemaphoreGive(mutexControl);
    }
}
}
bool prepararRegistro() {
    registroPreparado=false;
    bootPersistente=false;
    mutexControl=xSemaphoreCreateMutex(); colaSD=xQueueCreate(RegistroConfig::COLA,sizeof(RegistroFila));
    colaMonitor=xQueueCreate(RegistroConfig::COLA_MONITOR,sizeof(RegistroFila)); comandos=xQueueCreate(8,sizeof(Orden));
    Preferences p;
    if (p.begin("eco-registro",false)) {
        bootId=p.getULong64("boot",0)+1; bootPersistente=p.putULong64("boot",bootId)==sizeof(uint64_t); p.end();
    }
    if (!bootPersistente) bootId=(uint64_t(esp_random())<<32)|esp_random();
    registroPreparado=mutexControl && colaSD && colaMonitor && comandos && inicializarReloj();
    return registroPreparado;
}
void comenzarSesionRegistro() { nuevaSesion(); }
bool crearTareasRegistro() {
    if (!mutexControl || !colaSD || !colaMonitor || !comandos) return false;
    const bool captura=xTaskCreatePinnedToCore(tareaCaptura,"CapturaCSV",6144,nullptr,2,nullptr,0)==pdPASS;
    const bool sd=xTaskCreatePinnedToCore(tareaSD,"EscritoraSD",10240,nullptr,1,nullptr,0)==pdPASS;
    const bool monitor=xTaskCreatePinnedToCore(tareaMonitor,"MonitorCSV",6144,nullptr,1,nullptr,0)==pdPASS;
    xSemaphoreTake(mutexControl,portMAX_DELAY);
    control.capturaDisponible=captura; control.sdDisponible=sd; control.monitorDisponible=monitor;
    // Si falta captura o escritura, no anunciar una sesión que no puede guardar.
    // La escritora, si existe, vacía y cierra lo que ya hubiera llegado a la cola.
    if (!captura || !sd) {
        control.capturando=false;
        control.cerrar=sd;
        control.cerrado=!sd;
    }
    xSemaphoreGive(mutexControl);
    if (!captura || !sd) logSerial("[SD] registro incompleto: captura=%u escritora=%u monitor=%u\n",captura,sd,monitor);
    else if (!monitor) logSerial("[SD] monitor no disponible; el CSV sigue registrando.\n");
    return captura && sd && monitor;
}
bool procesarComandoRegistro(const char* s) {
    Orden o;
    if (!strcmp(s,"sd estado")) o=Orden::ESTADO;
    else if (!strcmp(s,"sd cerrar")) o=Orden::CERRAR;
    else if (!strcmp(s,"sd nueva")) o=Orden::NUEVA;
    else return false;
    if (!registroPreparado) { Serial.println("#DIAG SD no disponible: sin recursos de registro."); return true; }
    const Control estado=leerControl();
    if (!estado.capturaDisponible || !estado.sdDisponible) {
        // Energía ya posee el puerto serial al despachar el comando. Se responde
        // aquí porque, si falta una de estas tareas, nadie puede procesar la cola.
        Serial.print("#DIAG SD no disponible: captura="); Serial.print(estado.capturaDisponible);
        Serial.print(" escritora="); Serial.println(estado.sdDisponible);
        return true;
    }
    if (!comandos || xQueueSend(comandos,&o,0)!=pdPASS) Serial.println("#DIAG SD: orden rechazada; servicio no disponible/cola llena.");
    else Serial.println("#DIAG SD: orden recibida; ejecucion asincrona.");
    return true;
}
