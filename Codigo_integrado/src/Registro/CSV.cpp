#include "CSV.h"
#include "ConfigRegistro.h"
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
const char* cabeceraCSV() {
    return "schema_version,session_id,registro,timestamp_ms,fecha_arg,hora_arg,unix_ms,fuente_hora,edad_sync_ms,"
        "segmento_energia,modo_energia,sesion_energia_activa,lecturas_energia_disponibles,calibrando_corriente,tiempo_integrado_s,"
        "tension_V,corriente_A,potencia_W,corriente_media_1s_A,corriente_max_1s_A,potencia_media_1s_W,ventana_resumen_ms,cobertura_resumen_ms,"
        "ah_consumidos,ah_restantes,wh_consumidos,wh_restantes,soc_pct,"
        "velocidad_kmh,rpm,pulsos_ventana,pulsos_total,pcnt_muestra,pcnt_perdidas,temp1_C,temp2_C,temp_prom_C,"
        "gps_lat,gps_lon,gps_vel_kmh,gps_rumbo_deg,gps_satelites,gps_hdop,"
        "adc_tension_V,adc_corriente_V,salida_corriente_V,offset_corriente_V,"
        "estado_tension,edad_tension_ms,estado_corriente,edad_corriente_ms,estado_energia,edad_energia_ms,"
        "estado_pcnt,edad_pcnt_ms,estado_temp1,edad_temp1_ms,estado_temp2,edad_temp2_ms,"
        "estado_gps_posicion,edad_gps_posicion_ms,estado_gps_velocidad,edad_gps_velocidad_ms,"
        "estado_gps_rumbo,edad_gps_rumbo_ms,estado_gps_hdop,edad_gps_hdop_ms,"
        "estado_gps_satelites,edad_gps_satelites_ms,registros_perdidos,monitor_omitidos\n";
}
const char* nombreModoRegistro(uint8_t m) {
    static const char* const nombres[]={"FUENTES_ANALOGICAS","SENSORES_REALES","REFERENCIA_SIMULADA","VOLTAJE_ADC_INYECTADO","ACS_TENSION_FIJA"};
    return m<5 ? nombres[m] : "DESCONOCIDO";
}
namespace {
struct Salida { char* b; size_t n, pos; bool bien; unsigned campos; };
void campo(Salida& s, const char* fmt="", ...) {
    if (!s.bien) return;
    if (s.campos++) {
        if (s.pos+1>=s.n) { s.bien=false; return; }
        s.b[s.pos++]=',';
    }
    va_list a; va_start(a,fmt);
    const int k=vsnprintf(s.b+s.pos,s.n-s.pos,fmt,a); va_end(a);
    if (k<0 || static_cast<size_t>(k)>=s.n-s.pos) { s.bien=false; return; }
    s.pos+=k;
}
void real(Salida& s, double v, unsigned precision, bool valido=true) {
    if (valido && isfinite(v)) campo(s,"%.*f",int(precision),v); else campo(s);
}
void entero(Salida& s, uint64_t v, bool valido=true) {
    if (valido) campo(s,"%llu",static_cast<unsigned long long>(v)); else campo(s);
}
const char* calidad(bool visto, bool valido, uint64_t marca, uint64_t us, uint32_t fresco,
                    const char* invalido="INVALIDA") {
    if (!visto) return "SIN_DATO";
    if (!valido) return invalido;
    return us>=marca && (us-marca)/1000<=fresco ? "OK" : "ANTIGUA";
}
void estadoEdad(Salida& s, const char* estado, uint64_t marca, uint64_t us, bool visto) {
    campo(s,"%s",estado); entero(s,us>=marca ? (us-marca)/1000 : 0,visto);
}
}
size_t formatearCSV(const RegistroFila& f, char* buffer, size_t n) {
    if (!n) return 0;
    Salida s={buffer,n,0,true,0};
    const auto& d=f.snapshot.datos;
    const auto& e=d.infoEnergia;
    const auto& t=d.temperatura;
    const auto& g=d.gps;
    const auto& v=d.velocidad;
    const auto& r=f.snapshot.resumen;
    const uint64_t us=f.snapshot.capturaUs;
    campo(s,"1"); campo(s,"%s",f.sessionId); entero(s,f.registro); entero(s,f.timestampMs);
    const bool hora=f.hora.fuente!=FuenteHora::SIN_HORA;
    char fecha[16]={}, reloj[20]={};
    if (hora) formatearArgentina(f.hora.unixMs,fecha,sizeof(fecha),reloj,sizeof(reloj));
    campo(s,"%s",fecha); campo(s,"%s",reloj);
    if (hora) campo(s,"%lld",static_cast<long long>(f.hora.unixMs)); else campo(s);
    campo(s,"%s",nombreFuente(f.hora.fuente)); entero(s,f.hora.edadMs,hora);
    entero(s,e.segmento); campo(s,"%s",nombreModoRegistro(e.modo));
    entero(s,e.activa); entero(s,e.disponible); entero(s,e.corrienteUsada.calibrando);
    real(s,e.tiempoIntegradoS,3);
    real(s,e.estado.voltajeV,2,e.disponible); real(s,e.estado.corrienteA,3,e.disponible);
    real(s,e.estado.potenciaW,2,e.disponible);
    real(s,r.corrienteMedia,3,r.coberturaUs>0); real(s,r.corrienteMax,3,r.coberturaUs>0);
    real(s,r.potenciaMedia,2,r.coberturaUs>0); entero(s,r.ventanaUs/1000); entero(s,r.coberturaUs/1000);
    real(s,e.estado.ahConsumidos,4); real(s,e.estado.ahRestantes,4);
    real(s,e.estado.whConsumidos,2); real(s,e.estado.whRestantes,2); real(s,e.estado.porcentajeBateria,2);
    real(s,v.kmh,2,v.muestra>0); real(s,v.rpm,2,v.muestra>0);
    entero(s,v.pulsos,v.muestra>0); entero(s,v.totalPulsos,v.muestra>0);
    entero(s,v.muestra,v.muestra>0); entero(s,v.perdidas);
    real(s,t.temp1,2,t.visto1 && t.valida1); real(s,t.temp2,2,t.visto2 && t.valida2);
    real(s,t.promedio,2,t.valida1 && t.valida2);
    real(s,g.latitud,6,g.ubicacionValida); real(s,g.longitud,6,g.ubicacionValida);
    real(s,g.velocidadKmH,2,g.velocidadValida); real(s,g.rumboGrados,1,g.rumboValido);
    entero(s,g.satelites,g.satelitesValidos); real(s,g.hdop,2,g.hdopValido);
    real(s,e.tensionUsada.voltajePinV,3,e.tensionADC && e.tensionUsada.lista);
    const bool adcI=e.corrienteADC && e.corrienteUsada.lista && !e.corrienteUsada.calibrando;
    real(s,e.corrienteUsada.voltajePinV,3,adcI); real(s,e.corrienteUsada.voltajeSensorV,3,adcI);
    real(s,e.corrienteUsada.offsetSensorV,3,e.corrienteADC && e.capturaUs>0);
    estadoEdad(s,!e.capturaUs ? "SIN_DATO" : e.tensionADC ? calidad(e.tensionUsada.lista,isfinite(e.tensionUsada.voltajeBateria),e.tensionUs,us,RegistroConfig::FRESCO_ADC_MS) : "SIMULADA",e.tensionUs,us,e.tensionADC ? e.tensionUsada.lista : e.capturaUs>0);
    estadoEdad(s,!e.capturaUs ? "SIN_DATO" : e.corrienteUsada.calibrando ? "CALIBRANDO" : e.corrienteADC ? calidad(e.corrienteUsada.lista,isfinite(e.corrienteUsada.corrienteA),e.corrienteUs,us,RegistroConfig::FRESCO_ADC_MS) : "SIMULADA",e.corrienteUs,us,e.corrienteADC ? e.corrienteUsada.lista : e.capturaUs>0);
    estadoEdad(s,calidad(e.capturaUs>0,e.disponible,e.capturaUs,us,RegistroConfig::FRESCO_ENERGIA_MS,"NO_DISPONIBLE"),e.capturaUs,us,e.capturaUs>0);
    estadoEdad(s,calidad(v.muestra>0,true,v.capturaUs,us,RegistroConfig::FRESCO_PCNT_MS),v.capturaUs,us,v.muestra>0);
    estadoEdad(s,calidad(t.visto1,t.valida1,t.captura1Us,us,RegistroConfig::FRESCO_TEMP_MS,t.parasitaria1 ? "ERROR_ALIMENTACION" : "DESCONECTADO"),t.captura1Us,us,t.visto1);
    estadoEdad(s,calidad(t.visto2,t.valida2,t.captura2Us,us,RegistroConfig::FRESCO_TEMP_MS,t.parasitaria2 ? "ERROR_ALIMENTACION" : "DESCONECTADO"),t.captura2Us,us,t.visto2);
    estadoEdad(s,calidad(g.posicionUs>0,g.ubicacionValida,g.posicionUs,us,RegistroConfig::FRESCO_GPS_MS,"SIN_FIX"),g.posicionUs,us,g.posicionUs>0);
    estadoEdad(s,calidad(g.velocidadUs>0,g.velocidadValida,g.velocidadUs,us,RegistroConfig::FRESCO_GPS_MS),g.velocidadUs,us,g.velocidadUs>0);
    estadoEdad(s,calidad(g.rumboUs>0,g.rumboValido,g.rumboUs,us,RegistroConfig::FRESCO_GPS_MS,"SIN_RUMBO"),g.rumboUs,us,g.rumboUs>0);
    estadoEdad(s,calidad(g.hdopUs>0,g.hdopValido,g.hdopUs,us,RegistroConfig::FRESCO_GPS_MS),g.hdopUs,us,g.hdopUs>0);
    estadoEdad(s,calidad(g.satelitesUs>0,g.satelitesValidos,g.satelitesUs,us,RegistroConfig::FRESCO_GPS_MS),g.satelitesUs,us,g.satelitesUs>0);
    entero(s,f.perdidos); entero(s,f.omitidosMonitor);
    if (!s.bien || s.pos+2>n) { buffer[0]=0; return 0; }
    buffer[s.pos++]='\n'; buffer[s.pos]=0;
    return s.pos;
}
