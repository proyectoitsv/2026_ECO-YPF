#include "ColaDisplay.h"

namespace {
QueueHandle_t colaDisplay = nullptr;
}

bool inicializarColaDisplay() {
    if (!colaDisplay) colaDisplay = xQueueCreate(1, sizeof(EstadoDisplay));
    return colaDisplay != nullptr;
}

bool publicarEstadoDisplay(const EstadoDisplay& estado) {
    // Sobrescribe el dato anterior si el display todavía no lo leyó: importa
    // el porcentaje más reciente, no reproducir todos los porcentajes pasados.
    return colaDisplay && xQueueOverwrite(colaDisplay, &estado) == pdPASS;
}

bool recibirEstadoDisplay(EstadoDisplay& estado, TickType_t espera) {
    return colaDisplay && xQueueReceive(colaDisplay, &estado, espera) == pdTRUE;
}
