#pragma once
#include <stdint.h>
#include <stddef.h>
struct BackendArchivo {
    void* contexto;
    bool (*existe)(void*,const char*);
    bool (*abrir)(void*,const char*,bool append);
    size_t (*escribir)(void*,const char*,size_t);
    void (*sincronizar)(void*);
    bool (*error)(void*);
    void (*cerrar)(void*);
    bool (*renombrar)(void*,const char*,const char*);
};
struct ArchivoRegistro {
    char ruta[160] = {};
    bool abierto = false, incierto = false;
    uint64_t filas = 0;
};
bool crearArchivoRegistro(ArchivoRegistro&, const BackendArchivo&, int64_t inicioUnixMs,
                         uint32_t parte, bool largos, const char* cabecera);
bool escribirFilaArchivo(ArchivoRegistro&, const BackendArchivo&, const char*, size_t);
bool sincronizarArchivo(ArchivoRegistro&, const BackendArchivo&);
bool cerrarArchivo(ArchivoRegistro&, const BackendArchivo&);
bool fecharArchivo(ArchivoRegistro&, const BackendArchivo&, int64_t inicioUnixMs,
                   uint32_t parte, bool largos);
