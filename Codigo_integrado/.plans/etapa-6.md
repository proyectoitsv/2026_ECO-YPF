# Etapa 6 — Validación e integración

Leer primero `.plans/README.md` (contexto, hallazgos y decisiones de diseño).

1. `pio run` limpio (`pio run -t clean` antes) y revisar warnings (flash/RAM, `#error` no disparados, duplicados de símbolos).
2. Revisión estática de concurrencia: cada módulo `static` invocado por una sola tarea; Serial protegido; sin `delay()` fuera de `vTaskDelay`; ISR sin mutex bloqueantes.
3. Con hardware (si disponible; el usuario ejecuta `! pio run -t upload` y `pio device monitor`): log temporal de `uxTaskGetStackHighWaterMark` por tarea (margen ≥ 25 %), jitter de la tarea de corriente (5 ms), 10+ min sin WDT/reset, pulsos PCNT correctos, GPS y temperatura reportando, comando `calibrar/iniciar/pausar/estado` operativos.
4. Ajustar stacks/prioridades en `Rtos.h` según resultados.

## Al terminar
- Correr `pio run` (debe salir con código 0).
- Tildar la etapa en `.plans/README.md` y anotar notas de traspaso (qué se movió/creó, desvíos del plan, pendientes).
