# Etapa 5 — Tareas GPS y Temperatura

Leer primero `.plans/README.md` (contexto, hallazgos y decisiones de diseño).

1. `TareaGPS`: `inicializarGPS()` antes del bucle (puede bloquear esperando al GPS: aceptable en tarea; no bloquear `setup()`); bucle `actualizarGPS()` + impresión como en `main GPS.cpp`; publicar `DatosGPS`. Descartar `SerialUART1` (13/14) salvo que el usuario lo requiera (ejemplo comentado en el original) → registrar decisión.
2. `TareaTemperatura`: instancia única `GestorTemperatura(25)/(33)`; `inicializar()` previo; cada 1 s `solicitarTemperaturas` + `leerTemperatura(0)`, publicar promedio; respetar regla `!primerPulsoRecibido` vía constante.
Aceptación: `pio run` OK.

## Al terminar
- Correr `pio run` (debe salir con código 0).
- Tildar la etapa en `.plans/README.md` y anotar notas de traspaso (qué se movió/creó, desvíos del plan, pendientes).
