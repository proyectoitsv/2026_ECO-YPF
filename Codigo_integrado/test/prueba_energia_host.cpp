#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "../src/Sensores/Corriente/Corriente.h"
#include "../src/Sensores/Energia/Energia.h"

static bool cerca(float actual, float esperado, float tolerancia) {
    return fabsf(actual - esperado) <= tolerancia;
}

int main() {
    // Una hora virtual en pasos de un segundo, sin esperar una hora real.
    GestorEnergia energia(17.0f, 48.0f);
    const float corrienteA = corrienteDesdeVoltajeSensor(2.840f, 2.500f);
    EstadoEnergia estado = {};
    for (int i = 0; i < 3600; ++i) {
        estado = energia.actualizar(48.0f, corrienteA, 1.0f);
    }

    assert(cerca(corrienteA, 8.5f, 0.0001f));
    assert(cerca(estado.potenciaW, 408.0f, 0.05f));
    assert(cerca(estado.ahConsumidos, 8.5f, 0.01f));
    assert(cerca(estado.ahRestantes, 8.5f, 0.01f));
    assert(cerca(estado.whConsumidos, 408.0f, 0.05f));
    assert(cerca(estado.whRestantes, 408.0f, 0.05f));
    assert(cerca(estado.porcentajeBateria, 50.0f, 0.05f));

    // El tiempo variable también debe sumar exactamente una hora.
    GestorEnergia intervalosVariables(17.0f, 48.0f);
    for (int i = 0; i < 1200; ++i) {
        intervalosVariables.actualizar(48.0f, 8.5f, 0.5f);
        estado = intervalosVariables.actualizar(48.0f, 8.5f, 2.5f);
    }
    assert(cerca(estado.ahConsumidos, 8.5f, 0.01f));
    assert(cerca(estado.whConsumidos, 408.0f, 0.05f));

    printf("OK: 408 W, 8.5 Ah consumidos, 8.5 Ah restantes, "
           "408 Wh consumidos, 408 Wh restantes, 50 %% SOC\n");
    return 0;
}
