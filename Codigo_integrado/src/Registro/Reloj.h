#pragma once
#include "RelojCore.h"
struct EventoHora { uint64_t monoUs, numero; int64_t unixMs, ajusteMs; };
bool inicializarReloj();
void sincronizarUTC(const FechaUTC&, uint64_t monoUs);
HoraRegistro leerHora(uint64_t monoUs);
bool extraerEventoHora(EventoHora&);
uint32_t eventosHoraPerdidos();
