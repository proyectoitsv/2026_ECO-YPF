#include "MemoriaBateria.h"
#include <stdint.h>
#include <math.h>

namespace {
// Se escriben Ah y Wh juntos, con una sola llamada, para no recuperar un
// contador nuevo y otro viejo si se corta la alimentación durante el guardado.
struct ConsumoGuardado {
    uint32_t version;
    float ah;
    float wh;
};
constexpr uint32_t VERSION = 1;
constexpr char CLAVE[] = "consumo";
bool valido(const ConsumoGuardado& c) {
    return c.version == VERSION && isfinite(c.ah) && isfinite(c.wh) &&
           c.ah >= 0.0f && c.wh >= 0.0f;
}
}

bool MemoriaBateria::iniciar() {
    // Namespace propio: no modifica ni depende de la memoria del registrador.
    disponible_ = preferencias_.begin("eco-bateria", false);
    return disponible_;
}

MemoriaBateria::Carga MemoriaBateria::cargar(float& ah, float& wh) {
    if (!disponible_) return Carga::ERROR;
    if (!preferencias_.isKey(CLAVE)) return Carga::NUEVA;
    ConsumoGuardado c = {};
    if (preferencias_.getBytesLength(CLAVE) != sizeof(c) ||
        preferencias_.getBytes(CLAVE, &c, sizeof(c)) != sizeof(c) || !valido(c))
        return Carga::ERROR;
    ah = c.ah;
    wh = c.wh;
    return Carga::RECUPERADA;
}

bool MemoriaBateria::guardar(float ah, float wh) {
    const ConsumoGuardado c = {VERSION, ah, wh};
    if (!disponible_ || !valido(c)) return false;
    // Preferences confirma la escritura en NVS. No se usa SD ni se reinicia
    // el ESP32 si falla: la tarea informa el error y sigue midiendo.
    return preferencias_.putBytes(CLAVE, &c, sizeof(c)) == sizeof(c);
}
