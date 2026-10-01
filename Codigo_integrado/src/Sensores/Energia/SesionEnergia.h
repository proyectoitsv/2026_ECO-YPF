#ifndef SESION_ENERGIA_H
#define SESION_ENERGIA_H

#include <stdint.h>
#include "Energia.h"

// Control temporal de una prueba. No conoce ADC, pines ni calibraciones.
// El llamador entrega magnitudes físicas y un contador de milisegundos.
class SesionEnergia {
public:
    SesionEnergia(float capacidadAh, float tensionNominalV)
        : energia_(capacidadAh, tensionNominalV), ultimoMs_(0),
          activa_(false), anteriorDisponible_(false), segundosIntegrados_(0.0) {}

    void iniciar(uint32_t ahoraMs) {
        if (activa_) return;
        activa_ = true;
        ultimoMs_ = ahoraMs;
        anteriorDisponible_ = false;
    }
    void pausar(uint32_t ahoraMs) {
        activa_ = false;
        ultimoMs_ = ahoraMs;
        anteriorDisponible_ = false;
    }
    void reiniciar(uint32_t ahoraMs) {
        pausar(ahoraMs);
        energia_.reiniciar();
        segundosIntegrados_ = 0.0;
    }
    EstadoEnergia actualizar(float voltajeV, float corrienteA,
                             uint32_t ahoraMs, bool lecturaDisponible) {
        float dtSegundos = 0.0f;
        if (activa_ && anteriorDisponible_ && lecturaDisponible)
            dtSegundos = static_cast<float>(ahoraMs - ultimoMs_) / 1000.0f;
        segundosIntegrados_ += dtSegundos;
        // Restar contadores sin signo permite el desbordamiento de millis().
        // La espera inicial/calibración no se integra retrospectivamente.
        // Esta clase no comprueba rangos: el bool solo indica disponibilidad.
        ultimoMs_ = ahoraMs;
        anteriorDisponible_ = lecturaDisponible;
        return energia_.actualizar(lecturaDisponible ? voltajeV : 0.0f,
                                  lecturaDisponible ? corrienteA : 0.0f, dtSegundos);
    }
    bool activa() const { return activa_; }
    double segundosIntegrados() const { return segundosIntegrados_; }
    const EstadoEnergia& obtenerEstado() const { return energia_.obtenerEstado(); }

private:
    GestorEnergia energia_;
    uint32_t ultimoMs_;
    bool activa_;
    bool anteriorDisponible_;
    double segundosIntegrados_;
};
#endif
