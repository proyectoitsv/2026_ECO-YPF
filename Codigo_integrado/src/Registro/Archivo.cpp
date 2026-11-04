#include "Archivo.h"
#include "RelojCore.h"
#include <stdio.h>
#include <string.h>
namespace {
bool nombreLibre(char* ruta, size_t n, const BackendArchivo& b, int64_t utc, uint32_t parte, bool largos) {
    FechaUTC f={}; if (utc>0) f=descomponerUnix(utc-3LL*3600000);
    for (unsigned sufijo=1;sufijo<=999999;++sufijo) {
        int k;
        if (!largos) k=snprintf(ruta,n,"/R%07u.CSV",sufijo);
        else if (utc<=0) k=snprintf(ruta,n,"/Prueba_SIN_HORA_%06u_P%03u.csv",sufijo,parte);
        else k=snprintf(ruta,n,"/Prueba_%04d-%02d-%02d_%02d-%02d-%02d_%03u_P%03u.csv",f.anio,f.mes,f.dia,f.hora,f.minuto,f.segundo,sufijo,parte);
        if (k<0 || size_t(k)>=n) return false;
        // Busca un sufijo libre: jamás sobrescribe una carrera anterior.
        if (!b.existe(b.contexto,ruta)) return true;
    }
    return false;
}
}
bool sincronizarArchivo(ArchivoRegistro& a, const BackendArchivo& b) {
    if (!a.abierto) return false;
    b.sincronizar(b.contexto); // flush() es void; solo se consulta el error disponible.
    if (b.error(b.contexto)) { a.incierto=true; return false; }
    return true;
}
bool cerrarArchivo(ArchivoRegistro& a, const BackendArchivo& b) {
    if (!a.abierto) return !a.incierto;
    const bool ok=sincronizarArchivo(a,b);
    b.cerrar(b.contexto); a.abierto=false; return ok;
}
bool escribirFilaArchivo(ArchivoRegistro& a, const BackendArchivo& b, const char* fila, size_t n) {
    // La SD solo recibe filas terminadas en salto de línea y completas.
    if (!a.abierto || a.incierto || !n || fila[n-1]!='\n') return false;
    if (b.escribir(b.contexto,fila,n)!=n || b.error(b.contexto)) {
        // La parte dañada no se prolonga con la próxima fila.
        a.incierto=true; b.cerrar(b.contexto); a.abierto=false; return false;
    }
    ++a.filas; return true;
}
bool crearArchivoRegistro(ArchivoRegistro& a, const BackendArchivo& b, int64_t utc,
                         uint32_t parte, bool largos, const char* cabecera) {
    char ruta[sizeof(a.ruta)];
    if (!nombreLibre(ruta,sizeof(ruta),b,utc,parte,largos)) return false;
    if (!b.abrir(b.contexto,ruta,false)) return false;
    a=ArchivoRegistro{}; a.abierto=true; strcpy(a.ruta,ruta);
    if (!escribirFilaArchivo(a,b,cabecera,strlen(cabecera))) return false;
    a.filas=0; return true;
}
bool fecharArchivo(ArchivoRegistro& a, const BackendArchivo& b, int64_t utc,
                   uint32_t parte, bool largos) {
    // Cuando llega UTC cambia el nombre provisional tras cerrar y sincronizar.
    if (!a.abierto || a.incierto || utc<=0 || !largos) return false;
    char nuevo[sizeof(a.ruta)];
    if (!nombreLibre(nuevo,sizeof(nuevo),b,utc,parte,largos)) return false;
    if (!cerrarArchivo(a,b)) return false;
    bool renombrado=b.renombrar(b.contexto,a.ruta,nuevo);
    if (renombrado) strcpy(a.ruta,nuevo);
    a.abierto=b.abrir(b.contexto,a.ruta,true);
    if (!a.abierto) a.incierto=true;
    return renombrado && a.abierto;
}
