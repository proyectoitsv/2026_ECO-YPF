# Etapa 3 — Tareas Corriente y Energía (Sensor Corriente + main Energia)

Leer primero `.plans/README.md` (contexto, hallazgos y decisiones de diseño).

**Base: `src/main tension.cpp` (versión vigente, NO `main Energia.cpp`).** Usar `lecturasDisponibles`; validar solo `lista` (+ `!calibrando` en corriente), sin `valida` ni antigüedad.

1. `TareaCorriente`: antes del bucle replicar la parte de corriente del `setup()` de Energia (`inicializarSensorCorriente(calibrar)` según `Config::MODO`/`CALIBRACION_CORRIENTE`, helpers `usaCorrienteADC/usaACSReal`); en el bucle `actualizarSensorCorriente()` cada 5 ms, publicar `obtenerMedidaCorriente()`; atender bandera `recalibrar`.
2. `TareaEnergia`: `Config::errorConfiguracion()` (si hay error: imprimir y suspender la tarea, NO `return`), `inicializarTension()`, `sesion.reiniciar`, mensajes de arranque; bucle: tensión → validaciones con `MAX_ANTIGUEDAD_LECTURA_MS` → `sesion.actualizar` → `leerComandos()` → reporte periódico. Portar `imprimirEstado/ayuda/ejecutarComando/ejecutarPruebaPatron` sin cambiar lógica; `calibrar` pasa a levantar la bandera. Eliminar de esta tarea todo lo auxiliar (temp/GPS/ISR 26: `actualizarAuxiliares`, `cuentaPulsos`).
3. Reemplazar el `main Sensor Corriente` (API vieja/C3) por la API vigente; documentar que el comando `calibrar` sustituye al `'c'` serie.
Aceptación: `pio run` OK; sin `delay()` bloqueantes salvo `vTaskDelay*`.

## Al terminar
- Correr `pio run` (debe salir con código 0).
- Tildar la etapa en `.plans/README.md` y anotar notas de traspaso (qué se movió/creó, desvíos del plan, pendientes).
