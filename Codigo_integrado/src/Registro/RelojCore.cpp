#include "RelojCore.h"
#include <stdio.h>
static bool bisiesto(int y) { return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0); }
static int diasMes(int y, int m) {
    static const int dias[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    return dias[m-1] + (m == 2 && bisiesto(y));
}
bool convertirUnix(const FechaUTC& f, int64_t& valor) {
    // Antes de usar el GPS como calendario se revisan fecha, hora y bisiestos.
    if (f.anio < 2020 || f.anio > 2099 || f.mes < 1 || f.mes > 12 ||
        f.dia < 1 || f.dia > diasMes(f.anio,f.mes) || f.hora < 0 || f.hora > 23 ||
        f.minuto < 0 || f.minuto > 59 || f.segundo < 0 || f.segundo > 59 ||
        f.milisegundo < 0 || f.milisegundo > 999) return false;
    int64_t dias = 0;
    for (int y = 1970; y < f.anio; ++y) dias += bisiesto(y) ? 366 : 365;
    for (int m = 1; m < f.mes; ++m) dias += diasMes(f.anio,m);
    dias += f.dia - 1;
    valor = (((dias*24 + f.hora)*60 + f.minuto)*60 + f.segundo)*1000 + f.milisegundo;
    return true;
}
FechaUTC descomponerUnix(int64_t ms) {
    FechaUTC f = {};
    if (ms < 0) return f;
    f.milisegundo = ms % 1000;
    int64_t s = ms / 1000;
    f.segundo = s % 60; s /= 60;
    f.minuto = s % 60; s /= 60;
    f.hora = s % 24;
    int64_t d = s / 24;
    f.anio = 1970;
    while (d >= (bisiesto(f.anio) ? 366 : 365)) d -= bisiesto(f.anio++) ? 366 : 365;
    f.mes = 1;
    while (d >= diasMes(f.anio,f.mes)) { d -= diasMes(f.anio,f.mes); ++f.mes; }
    f.dia = d + 1;
    return f;
}
void formatearArgentina(int64_t ms, char* fecha, size_t nf, char* hora, size_t nh) {
    // El valor Unix queda en UTC; solo fecha/hora presentadas usan UTC−3.
    const FechaUTC f = descomponerUnix(ms - 3LL*3600000);
    snprintf(fecha,nf,"%02d/%02d/%04d",f.dia,f.mes,f.anio);
    snprintf(hora,nh,"%02d:%02d:%02d.%03d",f.hora,f.minuto,f.segundo,f.milisegundo);
}
HoraRegistro estimarHora(const ReferenciaUTC& r, uint64_t us, uint32_t fresco) {
    HoraRegistro h;
    // Nunca fechar hacia atrás con una referencia adquirida después de la foto.
    if (!r.valida || us < r.monoUs) return h;
    h.edadMs = (us-r.monoUs)/1000;
    h.unixMs = r.unixMs + static_cast<int64_t>(h.edadMs);
    h.fuente = h.edadMs <= fresco ? FuenteHora::GPS : FuenteHora::ESTIMADA;
    return h;
}
int64_t aplicarSincronizacion(ReferenciaUTC& r, int64_t ms, uint64_t us) {
    const int64_t delta = r.valida ? ms-(r.unixMs+static_cast<int64_t>((us-r.monoUs)/1000)) : 0;
    r.valida = true; r.unixMs = ms; r.monoUs = us; ++r.sincronizaciones;
    return delta;
}
const char* nombreFuente(FuenteHora f) {
    return f == FuenteHora::GPS ? "GPS" : f == FuenteHora::ESTIMADA ? "ESTIMADA" : "SIN_HORA";
}
