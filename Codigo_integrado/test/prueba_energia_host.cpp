#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../src/Sensores/Energia/SesionEnergia.h"
#include "../src/Sensores/Corriente/Corriente.h"
#include "../src/Sensores/Tension/Tension.h"

static bool cerca(float a, float b, float tolerancia = 0.001f) {
    return fabsf(a - b) < tolerancia;
}
int main() {
    assert(Config::errorConfiguracion() == nullptr);
    assert(cerca(tensionPackDesdeVoltajePin(0.450f), 48.0f));
    assert(cerca(tensionPackDesdeVoltajePin(0.850f), 50.0f));
    assert(cerca(tensionPackDesdeVoltajePin(1.250f), 52.0f));
    assert(cerca(corrienteDesdeVoltajeSensor(2.840f, 2.500f), 8.5f));
    assert(cerca(corrienteDesdeVoltajeSensor(2.500f, 2.500f), 0.0f));
    GestorEnergia energia(17.0f, 48.0f);
    EstadoEnergia e = {};
    // Intervalos distintos: cada pareja suma 3 segundos, total una hora.
    for (int i = 0; i < 1200; ++i) {
        energia.actualizar(48.0f, 8.5f, 0.5f);
        e = energia.actualizar(48.0f, 8.5f, 2.5f);
    }
    assert(cerca(e.potenciaW, 408.0f));
    assert(cerca(e.ahConsumidos, 8.5f));
    assert(cerca(e.ahRestantes, 8.5f));
    assert(cerca(e.whConsumidos, 408.0f));
    assert(cerca(e.whRestantes, 408.0f));
    assert(cerca(e.porcentajeBateria, 50.0f));
    printf("PATRON OK: 408 W, 8.5/8.5 Ah, 408/408 Wh, 50 %% SOC\n");

    GestorEnergia variable(17.0f, 48.0f);
    variable.actualizar(48.0f, 8.5f, 600.0f);
    e = variable.actualizar(52.0f, 5.0f, 300.0f);
    assert(cerca(e.ahConsumidos, 1.833333f));
    assert(cerca(e.whConsumidos, 89.666667f));
    assert(cerca(e.potenciaW, 260.0f));
    const float consumidos = e.ahConsumidos;
    e = variable.actualizar(52.0f, -5.0f, 60.0f);
    assert(cerca(e.potenciaW, -260.0f));
    assert(cerca(e.ahConsumidos, consumidos)); // Recuperación aún no implementada.
    e = variable.actualizar(48.0f, 50.0f, 3600.0f);
    assert(e.ahRestantes == 0.0f && e.whRestantes == 0.0f &&
           e.porcentajeBateria == 0.0f);

    SesionEnergia sesion(17.0f, 48.0f);
    e = sesion.actualizar(48.0f, 8.5f, 1000, true); // Arranque pausado.
    assert(e.ahConsumidos == 0.0f);
    sesion.iniciar(1000);
    sesion.actualizar(48.0f, 8.5f, 1000, true);
    e = sesion.actualizar(48.0f, 8.5f, 2000, true);
    const float unSegundoAh = 8.5f / 3600.0f;
    assert(cerca(e.ahConsumidos, unSegundoAh, 0.000001f));
    sesion.pausar(2000);
    e = sesion.actualizar(52.0f, 5.0f, 12000, true);
    assert(cerca(e.potenciaW, 260.0f));
    assert(cerca(e.ahConsumidos, unSegundoAh, 0.000001f));
    sesion.iniciar(12000);
    sesion.actualizar(48.0f, 8.5f, 12000, true);
    sesion.actualizar(48.0f, 8.5f, 13000, true);
    sesion.actualizar(0.0f, 0.0f, 14000, false); // Calibración sin datos disponibles.
    e = sesion.actualizar(48.0f, 8.5f, 24000, true);
    assert(cerca(e.ahConsumidos, 2 * unSegundoAh, 0.000001f));
    e = sesion.actualizar(48.0f, 8.5f, 25000, true);
    assert(cerca(e.ahConsumidos, 3 * unSegundoAh, 0.000001f));
    sesion.reiniciar(25000);
    assert(!sesion.activa() && sesion.segundosIntegrados() == 0.0);
    assert(sesion.obtenerEstado().ahConsumidos == 0.0f);
    assert(sesion.obtenerEstado().ahRestantes == 17.0f);

    // El contador de millis() pasa por cero aproximadamente cada 49,7 días.
    sesion.iniciar(UINT32_MAX - 500);
    sesion.actualizar(48.0f, 8.5f, UINT32_MAX - 500, true);
    e = sesion.actualizar(48.0f, 8.5f, 499, true);
    assert(cerca(e.ahConsumidos, unSegundoAh, 0.000001f));
    printf("OK: valores variables, limites SOC, pausa, reinicio, calibracion y millis().\n");
    return 0;
}
