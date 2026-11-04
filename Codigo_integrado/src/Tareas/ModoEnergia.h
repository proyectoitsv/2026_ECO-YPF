#ifndef MODO_ENERGIA_H
#define MODO_ENERGIA_H

#include "../Configuracion.h"

// Helpers de Config::MODO compartidos por las tareas Corriente y Energía.
inline bool usaTensionADC() {
    return Config::MODO == Config::ModoEnergia::FUENTES_ANALOGICAS ||
           Config::MODO == Config::ModoEnergia::SENSORES_REALES;
}
inline bool usaCorrienteADC() {
    return Config::MODO != Config::ModoEnergia::REFERENCIA_SIMULADA;
}
inline bool usaACSReal() {
    return Config::MODO == Config::ModoEnergia::SENSORES_REALES ||
           Config::MODO == Config::ModoEnergia::ACS_TENSION_FIJA;
}
inline bool calibraAlArrancar() {
    return usaACSReal() && Config::CALIBRACION_CORRIENTE ==
        Config::CalibracionCorriente::AUTOMATICA_EN_CERO;
}

#endif
