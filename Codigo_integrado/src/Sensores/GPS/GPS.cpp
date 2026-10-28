#include "GPS.h"
#include <TinyGPSPlus.h>
#include <esp_timer.h>
#include <string.h>
#include "../../Registro/Reloj.h"
static TinyGPSPlus gps;
// TinyGPS++ confirma fecha/hora incluso si el campo RMC vino vacío.
// Verificar también los campos recibidos impide reciclar una fecha anterior.
struct CamposUTC {
    TinyGPSCustom hora, fecha;
    CamposUTC(const char* sentencia) : hora(gps,sentencia,1), fecha(gps,sentencia,9) {}
};
static CamposUTC utcGP("GPRMC"),utcGN("GNRMC"),utcGA("GARMC"),utcGB("GBRMC"),utcGL("GLRMC");
static CamposUTC* const camposUTC[]={&utcGP,&utcGN,&utcGA,&utcGB,&utcGL};
static bool digitos(const char* s,size_t n) {
    for (size_t i=0;i<n;++i) if (s[i]<'0' || s[i]>'9') return false;
    return true;
}
static bool camposUTCCompletos() {
    bool completos=false;
    for (auto* c:camposUTC) {
        const bool ambos=c->hora.isUpdated() && c->fecha.isUpdated();
        const char* h=c->hora.value(); const char* f=c->fecha.value();
        const size_t n=strlen(h);
        bool hora=n>=6 && digitos(h,6);
        if (n>6) hora=hora && h[6]=='.' && n>7 && digitos(h+7,n-7);
        completos=completos || (ambos && hora && strlen(f)==6 && digitos(f,6));
    }
    return completos;
}
static DatosGPS datosActuales = {};
static bool nuevaLectura=false, ecoEnviado=false;
static uint32_t arranqueMs;
const byte UBX_ECO_MODE[] PROGMEM={0xB5,0x62,0x06,0x11,0x02,0x00,0x08,0x01,0x22,0x92};
void inicializarGPS() { Serial2.begin(9600,SERIAL_8N1,17,16); arranqueMs=millis(); }
static void sentenciaCompleta() {
    const uint64_t us=esp_timer_get_time();
    bool cambio=false;
    if (gps.location.isUpdated()) {
        datosActuales.ubicacionValida=gps.location.isValid();
        datosActuales.latitud=gps.location.lat(); datosActuales.longitud=gps.location.lng();
        datosActuales.posicionUs=us; cambio=true;
    }
    if (gps.speed.isUpdated()) {
        datosActuales.velocidadValida=gps.speed.isValid();
        float v=gps.speed.kmph(); datosActuales.velocidadKmH=v<2.0f ? 0.0f : v;
        datosActuales.velocidadUs=us; cambio=true;
    }
    if (gps.course.isUpdated()) {
        bool valido=gps.course.isValid(); datosActuales.rumboGrados=gps.course.deg();
        datosActuales.rumboValido=valido && datosActuales.velocidadValida && datosActuales.velocidadKmH>0 && us-datosActuales.velocidadUs<=3000000;
        datosActuales.rumboUs=us; cambio=true;
    }
    if (gps.hdop.isUpdated()) {
        datosActuales.hdopValido=gps.hdop.isValid(); datosActuales.hdop=gps.hdop.hdop();
        datosActuales.hdopUs=us; cambio=true;
    }
    if (gps.satellites.isUpdated()) {
        datosActuales.satelitesValidos=gps.satellites.isValid(); datosActuales.satelites=gps.satellites.value();
        datosActuales.satelitesUs=us; cambio=true;
    }
    // Consumir flags en CADA sentencia: no unir fecha vieja de RMC con hora de GGA.
    bool fechaNueva=gps.date.isUpdated(), horaNueva=gps.time.isUpdated();
    bool reciente=gps.date.age()<=1500 && gps.time.age()<=1500;
    bool valida=gps.date.isValid() && gps.time.isValid();
    FechaUTC f={int(gps.date.year()),int(gps.date.month()),int(gps.date.day()),int(gps.time.hour()),int(gps.time.minute()),int(gps.time.second()),int(gps.time.centisecond())*10};
    if (camposUTCCompletos() && fechaNueva && horaNueva && valida && reciente) sincronizarUTC(f,us);
    if (fechaNueva || horaNueva) {
        HoraRegistro h=leerHora(us);
        if (h.fuente!=FuenteHora::SIN_HORA) {
            FechaUTC a=descomponerUnix(h.unixMs-3LL*3600000);
            datosActuales.anio=a.anio; datosActuales.mes=a.mes; datosActuales.dia=a.dia;
            datosActuales.hora=a.hora; datosActuales.minuto=a.minuto; datosActuales.segundo=a.segundo;
        }
        cambio=true;
    }
    nuevaLectura=nuevaLectura || cambio;
}
void actualizarGPS() {
    while (Serial2.available()>0) if (gps.encode(Serial2.read())) sentenciaCompleta();
    if (!ecoEnviado && uint32_t(millis()-arranqueMs)>=2000) {
        for (size_t i=0;i<sizeof(UBX_ECO_MODE);++i) Serial2.write(pgm_read_byte(&UBX_ECO_MODE[i]));
        ecoEnviado=true;
    }
}
bool datosGPSActualizados() { bool r=nuevaLectura; nuevaLectura=false; return r; }
DatosGPS obtenerDatosGPS() { return datosActuales; }
