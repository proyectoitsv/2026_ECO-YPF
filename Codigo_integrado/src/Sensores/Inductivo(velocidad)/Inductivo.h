#ifndef INDUCTIVO_H
#define INDUCTIVO_H

#include <Arduino.h>

// Estructura para devolver los datos calculados
struct MedidaInductivo {
    float rpm;
    float velocidadKmH;
};

class GestorInductivo {
public:
    GestorInductivo(uint8_t pin, int pulsosPorRev, float diametroMetros);
    void inicializar();
    MedidaInductivo leerMedida();

private:
    uint8_t _pin;
    int _pulsosPorRev;
    float _diametroMetros;
    unsigned long _tiempoAnterior;
    float _rpmActual;
    float _velocidadActual;

    // Métodos y variables estáticas requeridas para la interrupción
    static volatile unsigned long contadorPulsos;
    static void IRAM_ATTR cuentaPulsos();
};

#endif