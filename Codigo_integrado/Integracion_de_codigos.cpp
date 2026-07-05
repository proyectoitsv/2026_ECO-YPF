#include "Tension.h"

MedidaTension leerTensionCompleta() {
    MedidaTension resultado;

    long suma = 0;
    for (int i = 0; i < 30; i++) {
        suma += analogRead(34);
        delay(2);
    }
    resultado.adc = (int)(suma / 30);

    resultado.voltaje = (resultado.adc * 3 / 4095.0);

    float voltajeLimpio = constrain(resultado.voltaje, 0.1, 3.1);
    resultado.porcentaje = ((voltajeLimpio - 0.1) / (3.1 - 0.1)) * 100.0;

    resultado.voltajeBateria = ((voltajeLimpio - 0.1) / (3.1 - 0.1)) * (56.0 - 48.0) + 48.0;
    
    resultado.adc_crudo = analogRead(34);
    return resultado;
}