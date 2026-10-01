# Etapa 0 — Prerrequisitos y línea base

Leer primero `.plans/README.md` (contexto, hallazgos y decisiones de diseño).

Objetivo: dejar el árbol de fuentes consistente y saber qué falta.
1. Mover (no copiar) a su ubicación esperada: `include/Configuracion.h → src/Configuracion.h`; `include/GPS.h → src/Sensores/GPS/GPS.h`; `include/SesionEnergia.h → src/Sensores/Energia/SesionEnergia.h`; `lib/Corriente.cpp → src/Sensores/Corriente/Corriente.cpp`; reemplazar `src/Sensores/Tension/Tension.h` por `include/Tension.h` (borrar el de `include/`); borrar `include/Corriente.h` (idéntico al de `src/`) y `Corriente.c` vacío.
2. Verificar presencia de: `GPS.cpp`, `Energia.h/.cpp`, `Tension.cpp` nuevo. **Si faltan: detener y pedirlos al usuario** (no inventarlos sin acuerdo; en ese caso ofrecer stubs mínimos con la API de los headers).
3. Identificar dependencia de GPS (`#include` de `GPS.cpp`, p. ej. TinyGPS++) y agregarla a `lib_deps`.
4. Fijar plataforma: `platform = espressif32@6.x` (Arduino-ESP32 2.0.x) para que `main PCNT` no dispare el `#error`.
5. Línea base: compilar cada sketch original por separado (copiándolo temporalmente a `src/main.cpp` en un entorno/carpeta de scratch) o al menos confirmar que los módulos `Sensores/*` compilan juntos.
Aceptación: lista de archivos presentes/faltantes documentada en `.plans/README.md`; módulos compilan.

## Al terminar
- Correr `pio run` (debe salir con código 0).
- Tildar la etapa en `.plans/README.md` y anotar notas de traspaso (qué se movió/creó, desvíos del plan, pendientes).
