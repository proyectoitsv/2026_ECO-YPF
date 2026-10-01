# Etapa 1 — platformio.ini y esqueleto de build

Leer primero `.plans/README.md` (contexto, hallazgos y decisiones de diseño).

1. En `platformio.ini` (env `esp32dev`): `build_src_filter = +<*> -<main?*.cpp>` (el patrón `?` coincide con el espacio; no coincide con `main.cpp`). Verificar con `pio run -v` que ningún `main *.cpp` se compile; si el glob falla, renombrar originales a `ref_*.cpp.txt` o moverlos a `referencia/` (decisión a registrar).
2. Mantener `build_flags = -I src/Sensores` (más `-I src` si hace falta), `monitor_speed`, `upload_port` tal cual.
3. Crear `src/main.cpp` mínimo (`setup` vacío con `Serial.begin`, `loop` con `vTaskDelete(NULL)`).
Aceptación: `pio run` OK.

## Al terminar
- Correr `pio run` (debe salir con código 0).
- Tildar la etapa en `.plans/README.md` y anotar notas de traspaso (qué se movió/creó, desvíos del plan, pendientes).
