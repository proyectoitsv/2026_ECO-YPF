#include "Energia.h"

#include <math.h>

namespace {
float limitar(float valor, float minimo, float maximo) {
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}

float valorFinitoO(float valor, float reemplazo) {
    return isfinite(valor) ? valor : reemplazo;
}
}

GestorEnergia::GestorEnergia(float capacidadPackAh,
                             float tensionNominalPackV)
    : capacidadPackAh_(capacidadPackAh > 0.0f ? capacidadPackAh : 0.0f),
      tensionNominalPackV_(tensionNominalPackV > 0.0f
                               ? tensionNominalPackV
                               : 0.0f),
      ahConsumidosAcumulados_(0.0),
      whConsumidosAcumulados_(0.0) {
    estado_.voltajeV = 0.0f;
    estado_.corrienteA = 0.0f;
    estado_.potenciaW = 0.0f;
    estado_.ahConsumidos = 0.0f;
    estado_.ahRestantes = capacidadPackAh_;
    estado_.whConsumidos = 0.0f;
    estado_.whRestantes = capacidadPackAh_ * tensionNominalPackV_;
    estado_.porcentajeBateria = capacidadPackAh_ > 0.0f ? 100.0f : 0.0f;
}

void GestorEnergia::reiniciar(double ahConsumidosIniciales,
                              double whConsumidosIniciales) {
    ahConsumidosAcumulados_ = ahConsumidosIniciales > 0.0
                                  ? ahConsumidosIniciales
                                  : 0.0;
    whConsumidosAcumulados_ = whConsumidosIniciales > 0.0
                                  ? whConsumidosIniciales
                                  : 0.0;
    actualizarValoresDerivados();
}

EstadoEnergia GestorEnergia::actualizar(float voltajeV,
                                        float corrienteA,
                                        float dtSegundos) {
    // Evita que una lectura inválida propague NaN o infinito al acumulador.
    voltajeV = valorFinitoO(voltajeV, 0.0f);
    corrienteA = valorFinitoO(corrienteA, 0.0f);
    dtSegundos = valorFinitoO(dtSegundos, 0.0f);

    estado_.voltajeV = voltajeV;
    estado_.corrienteA = corrienteA;
    estado_.potenciaW = voltajeV * corrienteA;

    // Esta primera versión cuenta solamente energía consumida en el sentido
    // positivo. La recuperación por corriente negativa queda para otra etapa.
    if (dtSegundos > 0.0f && corrienteA > 0.0f) {
        const double dtHoras = static_cast<double>(dtSegundos) / 3600.0;
        ahConsumidosAcumulados_ +=
            static_cast<double>(corrienteA) * dtHoras;

        // Ah se calcula solo desde corriente. Wh además necesita tensión válida.
        if (voltajeV > 0.0f) {
            whConsumidosAcumulados_ +=
                static_cast<double>(voltajeV) * corrienteA * dtHoras;
        }
    }

    actualizarValoresDerivados();
    return estado_;
}

const EstadoEnergia& GestorEnergia::obtenerEstado() const {
    return estado_;
}

void GestorEnergia::actualizarValoresDerivados() {
    const double energiaNominalWh =
        static_cast<double>(capacidadPackAh_) * tensionNominalPackV_;
    double ahRestantes =
        static_cast<double>(capacidadPackAh_) - ahConsumidosAcumulados_;
    double whRestantes = energiaNominalWh - whConsumidosAcumulados_;

    // Absorbe residuos numéricos muy pequeños al llegar al límite nominal.
    if (ahRestantes < 0.000001) ahRestantes = 0.0;
    if (whRestantes < 0.001) whRestantes = 0.0;
    if (ahRestantes > capacidadPackAh_) ahRestantes = capacidadPackAh_;
    if (whRestantes > energiaNominalWh) whRestantes = energiaNominalWh;

    estado_.ahConsumidos = static_cast<float>(ahConsumidosAcumulados_);
    estado_.whConsumidos = static_cast<float>(whConsumidosAcumulados_);
    estado_.ahRestantes = static_cast<float>(ahRestantes);
    estado_.whRestantes = static_cast<float>(whRestantes);

    if (capacidadPackAh_ > 0.0f) {
        const float soc =
            (estado_.ahRestantes / capacidadPackAh_) * 100.0f;
        estado_.porcentajeBateria = limitar(soc, 0.0f, 100.0f);
    } else {
        estado_.porcentajeBateria = 0.0f;
    }
}
