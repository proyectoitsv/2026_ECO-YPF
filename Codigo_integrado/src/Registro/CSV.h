#pragma once
#include "../Rtos/Compartidos.h"
#include "RelojCore.h"
#include <stddef.h>
#include <type_traits>
struct RegistroFila {
    SnapshotRegistro snapshot;
    HoraRegistro hora;
    uint64_t timestampMs, registro, perdidos, omitidosMonitor;
    char sessionId[56];
};
static_assert(std::is_trivially_copyable<RegistroFila>::value,"La cola exige una trama copiable por valor, sin punteros propietarios");
const char* cabeceraCSV();
const char* nombreModoRegistro(uint8_t);
// El mismo criterio de edad/validez sirve para el CSV y el panel serial.
const char* calidadRegistro(bool visto, bool valido, uint64_t marcaUs, uint64_t capturaUs,
                            uint32_t frescoMs, const char* estadoInvalido="INVALIDA");
// Devuelve 0 si el buffer no alcanza: nunca se debe escribir una fila cortada.
size_t formatearCSV(const RegistroFila&, char* buffer, size_t capacidad);
