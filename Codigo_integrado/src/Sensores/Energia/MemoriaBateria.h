#pragma once
#include <Preferences.h>

// Un único registro pequeño en la memoria interna del ESP32, independiente
// de la tarjeta SD. Solo la tarea Energia usa esta clase; no necesita mutex.
class MemoriaBateria {
public:
    enum class Carga { NUEVA, RECUPERADA, ERROR };
    bool iniciar();
    Carga cargar(float& ahConsumidos, float& whConsumidos);
    bool guardar(float ahConsumidos, float whConsumidos);

private:
    Preferences preferencias_;
    bool disponible_ = false;
};
