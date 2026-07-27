#include "Inductivo.h"

// Inicialización de la variable estática
volatile unsigned long GestorInductivo::contadorPulsos = 0;

void IRAM_ATTR GestorInductivo::cuentaPulsos() {
    contadorPulsos++;
}

GestorInductivo::GestorInductivo(uint8_t pin, int pulsosPorRev, float diametroMetros)
    : _pin(pin),
      _pulsosPorRev(pulsosPorRev),
      _diametroMetros(diametroMetros),
      _tiempoAnterior(0),
      _rpmActual(0.0f),
      _velocidadActual(0.0f)
{
}

void GestorInductivo::inicializar() {
    pinMode(_pin, INPUT);
    attachInterrupt(digitalPinToInterrupt(_pin), cuentaPulsos, RISING);
}

MedidaInductivo GestorInductivo::leerMedida() {

    unsigned long tiempoActual = millis();

    if (tiempoActual - _tiempoAnterior >= 1000) {

        noInterrupts();
        unsigned long pulsos = contadorPulsos;
        contadorPulsos = 0;
        interrupts();

        // Tiempo real transcurrido en segundos
        float tiempoSegundos = (tiempoActual - _tiempoAnterior) / 1000.0f;

        // Revoluciones por segundo
        float rps = (float)pulsos / (_pulsosPorRev * tiempoSegundos);

        // RPM
        _rpmActual = rps * 60.0f;

        // Circunferencia de la rueda (m)
        float circunferencia = PI * _diametroMetros;

        // Velocidad (km/h)
        _velocidadActual = rps * circunferencia * 3.6f;

        _tiempoAnterior = tiempoActual;
    }

    MedidaInductivo datos;
    datos.rpm = _rpmActual;
    datos.velocidadKmH = _velocidadActual;

    return datos;
}