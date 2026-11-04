#pragma once
#include "CSV.h"

// Estado administrativo tomado al imprimir; las mediciones siempre vienen
// de la misma captura que se envía a la SD.
struct EstadoPanelRegistro {
    bool capturando, cerrado, sdDisponible, monitorDisponible, metaPendiente;
    uint64_t escritos, perdidos, omitidos;
    const char* ultimoError;
};

// Devuelve cero si el panel no entra entero. Nunca se manda medio mensaje.
size_t formatearPanelRegistro(const RegistroFila&, const EstadoPanelRegistro&,
                             char* salida, size_t capacidad);
