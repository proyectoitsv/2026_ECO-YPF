# Etapa 2 — Infraestructura RTOS

Leer primero `.plans/README.md` (contexto, hallazgos y decisiones de diseño).

1. `src/Rtos/Rtos.h`: constantes (prioridades, stacks, cores, periodos, flags `HABILITAR_*`), `extern SemaphoreHandle_t mtxSerial`, macros/helpers `LOG_LOCK()/LOG_UNLOCK()` o `logf()`.
2. `src/Rtos/Compartidos.h/.cpp`: `DatosCompartidos` (MedidaCorriente, MedidaTension, EstadoEnergia, velocidad rpm/kmh/pulsos, DatosGPS, temp1/temp2/promedio, flags `primerPulsoRecibido`, `solicitudCalibrar`), mutex y accesores `publicar*/leer*`.
3. `main.cpp`: crea mutexes y tareas con `xTaskCreatePinnedToCore` (revisar retorno; si falla, log + `ESP.restart()` o parar).
4. Tareas esqueleto con heartbeat para validar scheduling.
Aceptación: `pio run` OK; tareas stub definidas con la tabla de arriba.

## Al terminar
- Correr `pio run` (debe salir con código 0).
- Tildar la etapa en `.plans/README.md` y anotar notas de traspaso (qué se movió/creó, desvíos del plan, pendientes).
