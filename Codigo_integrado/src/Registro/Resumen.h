#pragma once
#include <stdint.h>
struct ResumenElectrico {
    // Promedios para analizar el tramo anterior entre dos capturas CSV.
    double corrienteMedia = 0, potenciaMedia = 0;
    float corrienteMax = 0;
    uint64_t ventanaUs = 0, coberturaUs = 0;
};
struct AcumuladorElectrico {
    // IntegralI/P suman valor × microsegundos; cobertura cuenta solo tiempo válido.
    uint64_t inicio = 0, ultimo = 0, validoHasta = 0, cobertura = 0;
    double integralI = 0, integralP = 0;
    float i = 0, p = 0, maximo = 0;
    bool iniciado = false, valido = false, hayMaximo = false;
};
void avanzarResumen(AcumuladorElectrico&, uint64_t us);
void actualizarResumen(AcumuladorElectrico&, uint64_t us, float i, float p,
                       bool valido, uint64_t validoHasta);
ResumenElectrico cerrarResumen(AcumuladorElectrico&, uint64_t us);
