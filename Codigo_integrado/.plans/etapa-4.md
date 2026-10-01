# Etapa 4 — Tarea PCNT (velocidad)

Leer primero `.plans/README.md` (contexto, hallazgos y decisiones de diseño).

1. Portar sin alterar constantes/ISR/buffer: `configurarPCNT()`, `configurarTimer()`, `onTimer` (IRAM_ATTR), `extraerMuestra`, `procesarMuestra`. Todo el `setup()` de PCNT (pinBaseTiempo, `pcnt_counter_clear/resume`, `timerStart`) va antes del `for(;;)` en la tarea pineada al core 1.
2. Loop: vaciar muestras → publicar rpm/km/h/pulsos/total; aviso de descartadas; `primerPulsoRecibido` compartido.
3. Quitar de esta tarea la tensión (`leerTensionCompleta`, ahora en Energía) y la temperatura (tarea propia). `GPIO12`: dejar configurable/desactivable (strapping).
4. Mantener `#if ESP_ARDUINO_VERSION_MAJOR != 2` y chequeo de target.
Aceptación: `pio run` OK; ISR sigue en IRAM, sin llamadas a Flash.

## Al terminar
- Correr `pio run` (debe salir con código 0).
- Tildar la etapa en `.plans/README.md` y anotar notas de traspaso (qué se movió/creó, desvíos del plan, pendientes).
