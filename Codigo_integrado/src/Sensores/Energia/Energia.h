#ifndef ENERGIA_H
#define ENERGIA_H

// Valores energéticos calculados a partir de magnitudes físicas.
// Este módulo no lee ADC ni conoce pines o sensores específicos.
struct EstadoEnergia {
    float voltajeV;
    float corrienteA;
    float potenciaW;
    float ahConsumidos;
    float ahRestantes;
    float whConsumidos;
    float whRestantes;
    float porcentajeBateria;
};

class GestorEnergia {
public:
    // Para el pack actual: 4 baterías en serie = 17 Ah y 48 V nominales.
    GestorEnergia(float capacidadPackAh,
                  float tensionNominalPackV);

    // Permite iniciar desde cero o restaurar acumulados en una versión futura.
    void reiniciar(double ahConsumidosIniciales = 0.0,
                   double whConsumidosIniciales = 0.0);

    // Integra el consumo durante dtSegundos usando valores físicos ya medidos.
    // La corriente negativa se informa como corriente/potencia instantánea,
    // pero todavía no se contabiliza como recuperación de energía.
    EstadoEnergia actualizar(float voltajeV,
                             float corrienteA,
                             float dtSegundos);

    const EstadoEnergia& obtenerEstado() const;

private:
    float capacidadPackAh_;
    float tensionNominalPackV_;
    double ahConsumidosAcumulados_;
    double whConsumidosAcumulados_;
    EstadoEnergia estado_;

    void actualizarValoresDerivados();
};

#endif
