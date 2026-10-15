#include "ColaDisplay.h"

namespace {
QueueHandle_t colaDisplay = nullptr;
}

bool inicializarColaDisplay() {
    if (!colaDisplay) colaDisplay = xQueueCreate(1, sizeof(EstadoDisplay));
    return colaDisplay != nullptr;
}

bool publicarEstadoDisplay(const EstadoDisplay& estado) {
    return colaDisplay && xQueueOverwrite(colaDisplay, &estado) == pdPASS;
}

bool recibirEstadoDisplay(EstadoDisplay& estado, TickType_t espera) {
    return colaDisplay && xQueueReceive(colaDisplay, &estado, espera) == pdTRUE;
}
