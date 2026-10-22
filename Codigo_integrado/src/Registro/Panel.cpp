#include "Panel.h"
#include "ConfigRegistro.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace {
struct Panel {
    char* texto;
    size_t capacidad, usados;
    bool completo;
};

void agregar(Panel& p, const char* formato, ...) {
    if (!p.completo) return;
    va_list argumentos;
    va_start(argumentos, formato);
    const int n=vsnprintf(p.texto+p.usados,p.capacidad-p.usados,formato,argumentos);
    va_end(argumentos);
    if (n<0 || size_t(n)>=p.capacidad-p.usados) { p.completo=false; return; }
    p.usados+=size_t(n);
}

// ANTIGUA conserva su último número para el análisis, pero lo rotula como viejo.
// Los estados sin lectura válida muestran solo la causa y nunca un cero inventado.
void numero(Panel& p, const char* titulo, double valor, unsigned decimales,
            const char* unidad, const char* estado) {
    if (isfinite(valor) && (!strcmp(estado,"OK") || !strcmp(estado,"SIMULADA") || !strcmp(estado,"ANTIGUA"))) {
        agregar(p,"%-19s %.*f %s",titulo,int(decimales),valor,unidad);
        if (strcmp(estado,"OK")) agregar(p," (%s)",estado);
        agregar(p,"\n");
    } else {
        const bool estadoValido=!strcmp(estado,"OK") || !strcmp(estado,"SIMULADA") || !strcmp(estado,"ANTIGUA");
        agregar(p,"%-19s %s\n",titulo,estadoValido && !isfinite(valor) ? "INVALIDA" : estado);
    }
}
}

size_t formatearPanelRegistro(const RegistroFila& fila, const EstadoPanelRegistro& registro,
                             char* salida, size_t capacidad) {
    if (!capacidad) return 0;
    Panel p={salida,capacidad,0,true};
    const auto& datos=fila.snapshot.datos;
    const auto& energia=datos.infoEnergia;
    const auto& gps=datos.gps;
    const auto& movimiento=datos.velocidad;
    const auto& temperaturas=datos.temperatura;
    const uint64_t us=fila.snapshot.capturaUs;
    const char* estadoEnergia=calidadRegistro(energia.capturaUs>0,energia.disponible,
        energia.capturaUs,us,RegistroConfig::FRESCO_ENERGIA_MS,"NO_DISPONIBLE");
    const char* estadoTension=!energia.capturaUs ? "SIN_DATO" : !energia.tensionADC ? "SIMULADA" :
        calidadRegistro(energia.tensionUsada.lista,isfinite(energia.tensionUsada.voltajeBateria),
                        energia.tensionUs,us,RegistroConfig::FRESCO_ADC_MS);
    const char* estadoCorriente=!energia.capturaUs ? "SIN_DATO" : energia.corrienteUsada.calibrando ?
        "CALIBRANDO" : !energia.corrienteADC ? "SIMULADA" :
        calidadRegistro(energia.corrienteUsada.lista,isfinite(energia.corrienteUsada.corrienteA),
                        energia.corrienteUs,us,RegistroConfig::FRESCO_ADC_MS);
    const char* estadoPotencia=estadoEnergia;
    if (!strcmp(estadoEnergia,"OK") && (!strcmp(estadoTension,"ANTIGUA") || !strcmp(estadoCorriente,"ANTIGUA")))
        estadoPotencia="ANTIGUA"; // El cálculo es reciente, alguna de sus entradas no.

    agregar(p,"\n========== ECO YPF 2026 ==========\n");
    agregar(p,"Sesión %s | fila %llu | tiempo %.1f s\n",fila.sessionId,
           static_cast<unsigned long long>(fila.registro),fila.timestampMs/1000.0);
    if (fila.hora.fuente==FuenteHora::SIN_HORA) agregar(p,"Hora Argentina: SIN_HORA\n");
    else {
        char fecha[16],hora[20];
        formatearArgentina(fila.hora.unixMs,fecha,sizeof(fecha),hora,sizeof(hora));
        agregar(p,"Hora Argentina: %s %s (%s)\n",fecha,hora,nombreFuente(fila.hora.fuente));
    }
    agregar(p,"Energía: %s | modo: %s\n",energia.activa ? "ACTIVA" : "PAUSADA",
           nombreModoRegistro(energia.modo));
    agregar(p,"BATERÍAS\n");
    numero(p,"Tensión:",energia.estado.voltajeV,2,"V",energia.disponible ? estadoTension : estadoEnergia);
    numero(p,"Corriente:",energia.estado.corrienteA,3,"A",energia.disponible ? estadoCorriente : estadoEnergia);
    numero(p,"Potencia:",energia.estado.potenciaW,2,"W",estadoPotencia);
    // Los acumulados son el estado del modelo, aun si se pausó la sesión.
    const char* acumulados=energia.capturaUs ? (!strcmp(estadoEnergia,"ANTIGUA") ? "ANTIGUA" : "OK") : "SIN_DATO";
    numero(p,"SOC del modelo:",energia.estado.porcentajeBateria,2,"%",acumulados);
    numero(p,"Ah consumidos:",energia.estado.ahConsumidos,4,"Ah",acumulados);
    numero(p,"Wh consumidos:",energia.estado.whConsumidos,2,"Wh",acumulados);

    agregar(p,"MOVIMIENTO\n");
    const char* estadoPCNT=calidadRegistro(movimiento.muestra>0,true,movimiento.capturaUs,us,RegistroConfig::FRESCO_PCNT_MS);
    numero(p,"Rueda:",movimiento.kmh,2,"km/h",estadoPCNT);
    numero(p,"RPM:",movimiento.rpm,2,"rpm",estadoPCNT);
    agregar(p,"TEMPERATURAS\n");
    numero(p,"Batería 1:",temperaturas.temp1,2,"°C",calidadRegistro(temperaturas.visto1,temperaturas.valida1,
        temperaturas.captura1Us,us,RegistroConfig::FRESCO_TEMP_MS,temperaturas.parasitaria1 ? "ERROR_ALIMENTACION" : "DESCONECTADO"));
    numero(p,"Batería 2:",temperaturas.temp2,2,"°C",calidadRegistro(temperaturas.visto2,temperaturas.valida2,
        temperaturas.captura2Us,us,RegistroConfig::FRESCO_TEMP_MS,temperaturas.parasitaria2 ? "ERROR_ALIMENTACION" : "DESCONECTADO"));
    agregar(p,"GPS\n");
    const char* posicion=calidadRegistro(gps.posicionUs>0,gps.ubicacionValida,gps.posicionUs,us,
                                         RegistroConfig::FRESCO_GPS_MS,"SIN_FIX");
    numero(p,"Latitud:",gps.latitud,6,"°",posicion);
    numero(p,"Longitud:",gps.longitud,6,"°",posicion);
    numero(p,"Velocidad GPS:",gps.velocidadKmH,2,"km/h",calidadRegistro(gps.velocidadUs>0,gps.velocidadValida,
        gps.velocidadUs,us,RegistroConfig::FRESCO_GPS_MS));
    agregar(p,"REGISTRO SD\n");
    agregar(p,"Captura: %s | escritor: %s | metadatos: %s\n",
           registro.capturando ? "ACTIVA" : registro.cerrado ? "CERRADA" : "DETENIDA",
           registro.sdDisponible ? "DISPONIBLE" : "NO DISPONIBLE",
           registro.metaPendiente ? "REINTENTO PENDIENTE" : "SIN FALLO PENDIENTE");
    agregar(p,"Filas guardadas: %llu | perdidas: %llu\n",
           static_cast<unsigned long long>(registro.escritos),static_cast<unsigned long long>(registro.perdidos));
    if (registro.ultimoError && registro.ultimoError[0])
        agregar(p,"Último error registrado: %s\n",registro.ultimoError);
    agregar(p,"==================================\n");
    if (!p.completo) { salida[0]=0; return 0; }
    return p.usados;
}
