#pragma once
#include <stdint.h>
#include <stddef.h>
struct FechaUTC { int anio, mes, dia, hora, minuto, segundo, milisegundo; };
bool convertirUnix(const FechaUTC&, int64_t& unixMs);
FechaUTC descomponerUnix(int64_t unixMs);
void formatearArgentina(int64_t unixMs, char* fecha, size_t nf, char* hora, size_t nh);
enum class FuenteHora : uint8_t { SIN_HORA, GPS, ESTIMADA };
struct ReferenciaUTC {
    bool valida = false;
    int64_t unixMs = 0;
    uint64_t monoUs = 0;
    uint64_t sincronizaciones = 0;
};
struct HoraRegistro {
    FuenteHora fuente = FuenteHora::SIN_HORA;
    int64_t unixMs = 0;
    uint64_t edadMs = 0;
};
HoraRegistro estimarHora(const ReferenciaUTC&, uint64_t ahoraUs, uint32_t frescoMs);
int64_t aplicarSincronizacion(ReferenciaUTC&, int64_t unixMs, uint64_t monoUs);
const char* nombreFuente(FuenteHora);
