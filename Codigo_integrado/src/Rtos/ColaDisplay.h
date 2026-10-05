#ifndef COLA_DISPLAY_H
#define COLA_DISPLAY_H

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Estado completo que quiere mostrar Energía. Se copia por valor en la cola.
// El modo manual viaja junto al SOC para no perder órdenes por sobrescritura.
struct EstadoDisplay {
    float porcentajeBateria;
    bool lecturaDisponible; // Primer promedio listo y sin calibración en curso.
    bool modoPrueba;
    int numeroPrueba;
};

// Llamar una vez en setup(), antes de crear las tareas.
bool inicializarColaDisplay();

// Cola de longitud 1: conserva el estado más reciente sin esperar espacio.
// Productor único: TareaEnergia. Consumidor único: TareaDisplay.
bool publicarEstadoDisplay(const EstadoDisplay& estado);
bool recibirEstadoDisplay(EstadoDisplay& estado, TickType_t espera);

#endif
