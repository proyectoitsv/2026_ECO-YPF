#include "Resumen.h"
#include <math.h>
#include <algorithm>
void avanzarResumen(AcumuladorElectrico& a, uint64_t us) {
    if (!a.iniciado) { a.inicio = a.ultimo = us; a.iniciado = true; }
    const uint64_t fin = std::min(us,a.validoHasta);
    if (a.valido && fin > a.ultimo) {
        const uint64_t dt = fin-a.ultimo;
        a.integralI += static_cast<double>(a.i)*dt;
        a.integralP += static_cast<double>(a.p)*dt;
        a.cobertura += dt;
        if (!a.hayMaximo || a.i > a.maximo) a.maximo = a.i;
        a.hayMaximo = true;
    }
    a.ultimo = us;
}
void actualizarResumen(AcumuladorElectrico& a, uint64_t us, float i, float p,
                       bool valido, uint64_t hasta) {
    avanzarResumen(a,us);
    a.i = i; a.p = p; a.validoHasta = hasta;
    a.valido = valido && isfinite(i) && isfinite(p) && hasta > us;
}
ResumenElectrico cerrarResumen(AcumuladorElectrico& a, uint64_t us) {
    avanzarResumen(a,us);
    ResumenElectrico r;
    r.ventanaUs = us-a.inicio; r.coberturaUs = a.cobertura;
    if (a.cobertura) { r.corrienteMedia=a.integralI/a.cobertura; r.potenciaMedia=a.integralP/a.cobertura; }
    r.corrienteMax = a.maximo;
    a.inicio = us; a.cobertura = 0; a.integralI = a.integralP = 0; a.hayMaximo = false;
    return r;
}
