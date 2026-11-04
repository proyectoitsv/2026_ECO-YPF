#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <map>
#include <string>
#include <vector>
namespace NvsSimulada {
extern std::map<std::string, std::vector<uint8_t>> datos;
extern unsigned escrituras;
extern bool fallaApertura, fallaEscritura;
}
class Preferences {
    std::string espacio_;
public:
    bool begin(const char* nombre, bool) {
        espacio_ = nombre;
        return !NvsSimulada::fallaApertura;
    }
    bool isKey(const char* clave) {
        return NvsSimulada::datos.count(espacio_ + "/" + clave) != 0;
    }
    size_t getBytesLength(const char* clave) {
        return NvsSimulada::datos[espacio_ + "/" + clave].size();
    }
    size_t getBytes(const char* clave, void* salida, size_t n) {
        const auto& dato = NvsSimulada::datos[espacio_ + "/" + clave];
        if (n < dato.size()) return 0;
        memcpy(salida, dato.data(), dato.size());
        return dato.size();
    }
    size_t putBytes(const char* clave, const void* entrada, size_t n) {
        if (NvsSimulada::fallaEscritura) return 0;
        const auto* p = static_cast<const uint8_t*>(entrada);
        NvsSimulada::datos[espacio_ + "/" + clave] = {p, p + n};
        ++NvsSimulada::escrituras;
        return n;
    }
};
