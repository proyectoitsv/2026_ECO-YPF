# Verificación del registro microSD

## Punto de partida

- Rama local: `feature/registro-microsd`.
- Base consultada: `Display-Sparkfun` en `e4ca276507141481e6e8dc57f8706e6747a8eb69` (`Display Encendido 2 Y Apagado 3`, 14/10/2026).
- Plataforma exacta compilada: espressif32 6.9.0, ESP32 Dev Module, Arduino-ESP32 2.0.17, GCC Xtensa 8.4.0, esptool 4.5.1; OneWire 2.3.8, DallasTemperature 3.11.0, TinyGPSPlus 1.1.0.
- `pio run`: **SUCCESS**, firmware binario generado. RAM estática indicada por PlatformIO: **25 036 / 327 680 bytes (7,6 %)**. Flash: **399 393 / 1 310 720 bytes (30,5 %)**.
- `TareaDisplay.cpp`, driver Display_COM11441 y módulos puros de cálculo de energía, corriente y tensión se compararon con la base: sin diferencias. La compilación/fixtures no reemplazan una prueba sobre el auto.

## Pruebas de software

Reproducir desde `Codigo_integrado` luego de instalar PlatformIO/dependencias:

```sh
pio run
python3 test/ejecutar_pruebas.py
```

Resultado ejecutado: **5 grupos aprobados**. Los binarios de PC se compilaron con C++11, `-Wall -Wextra -Werror`, y se eliminaron al finalizar.

1. Núcleo: fecha gregoriana/bisiestos, cambio Argentina UTC−3 en fecha/año, fecha inválida, reloj sin hora/GPS/estimado/reajuste, media ponderada, potencia, máximo observado, cobertura y expiración; nombres únicos, 8.3, renombrado y reapertura, fallo de rename, escritura parcial sin append, CSV vacío/cero y recuperación energética 48 V, 8,5 A, 3600 s = 408 W, 8,5 Ah, 408 Wh, 50 %.
2. Tareas reales de captura/escritura (`Registro.cpp`) con reloj, colas, Arduino y SD simulados: timestamps de captura conservados, cola de 32 que descarta registros nuevos, ausencia/reintento SD, creación de archivo, fila parcial/parte de recuperación, cierre, sesión nueva y acumulados de energía conservados; tarjeta llena retiene la cola, no crea partes vacías, cierre la contabiliza y espacio restaurado permite escribir. El test recorre el código de producción; la simulación no mide latencia física.
3. Regresión del flujo Energía→cola→Display: SOC 50 %, UART, comandos/manual, límites, estados y no repetir valores. Los archivos de display no se modificaron; la prueba usa mocks temporales para FreeRTOS/UART.
4. Parser TinyGPSPlus 1.1.0 y wrapper real con NMEA/checksums, UART simulada: RMC con hora pero sin fix, cambio medianoche, talker GP/GN, RMC con fecha vacía rechazada, sin mezclar fecha antigua, cero detenido sin rumbo, no renovar antigüedad de posición ante GGA sin fix, hora estimada/recuperación y comando ECO diferido sin espera de dos segundos.
5. Tarea DS18B20 real con conversores/FreeRTOS simulados: conversiones paralelas a 12 bits, periodo de 5 s con independencia del movimiento, desconexión individual, promedio vacío y alarma de alimentación parasitaria.

La prueba de CSV valida 71 celdas en cada fila de `datos_SIMULADOS.csv`; la cabecera mide 1057 bytes. Con el conjunto representativo sintético, el formateador produjo **381 bytes/fila** (CSV sintético usa `session_id` de 26 caracteres). Para estimar el ID real generado de 34 caracteres, se agregan 8 bytes: **389 bytes/fila estimados**. Un stress no físico de los anchos límite de tipos produjo 1699 bytes y permanece bajo `MAX_CSV=2048`. Dos coordenadas `double` deliberadamente fuera de rango producen rechazo completo si la fila supera el buffer; no se escriben filas truncadas. Esto comprueba formato y límite, no el máximo valor posible de cualquier double finito.

## RAM y almacenamiento

`RegistroFila` se midió con DWARF del objeto ESP32 compilado: **584 bytes** (también 584 en host). Es trivially copyable. Memoria nominal de elementos de colas: 32×584 + 2×584 = **19 856 bytes**, más control/órdenes de FreeRTOS. Las tres pilas nuevas solicitadas ocupan 6144 + 10240 + 6144 = **22 528 bytes**. Las pilas existentes de adquisición conservan sus dimensiones y no se incluyen en el subtotal nuevo. Los buffers de fila/metadatos están dentro de la pila de SD/monitor; el máximo real de stack, heap libre, colas y tiempos debe leerse en `sd estado` sobre el ESP32 con las bibliotecas/tarjeta reales.

Estimación a 1 fila/s, usando 389 bytes promedio (solo filas; sin sobrecarga interna FAT):

| Duración | Filas | CSV de datos | Cabecera, una vez |
| --- | ---: | ---: | ---: |
| 1 h | 3 600 | 1 400 400 B (1,34 MiB) | 1 057 B |
| 3 h | 10 800 | 4 201 200 B (4,01 MiB) | 1 057 B |
| 6 h | 21 600 | 8 402 400 B (8,01 MiB) | 1 057 B |

Un sidecar sintético completo midió 1487 bytes, con configuración y cierre; en hardware su tamaño cambia con el `session_id`, partes, renombres y eventos de reloj. No es un máximo fijo. Añadir margen para FAT, sidecar, fragmentación y contingencias; FAT32 de 1 GB alcanza holgadamente la estimación de 6 horas.

## Lo pendiente en hardware

No se ha conectado ESP32, lector SD, tarjeta, GPS, DS18B20 ni vehículo para esta validación. Probar:

- VCC real del módulo, nivel lógico 3,3 V, CS GPIO5 durante arranque, FAT32 y contactos/cables.
- `sd estado`: heap mínimo, high-water marks de los tres stacks, máximo de cola, máximos de escritura/sync. Repetir con tarjeta lenta y monitor activo.
- Frecuencias efectivas de corrientes/promedios, temperatura 5 s, NMEA/época GPS, captura 1 Hz y jitter de escritura. El objetivo del receptor GPS de 1 Hz requiere medir el modelo/configuración instalada.
- Retiro de tarjeta, tarjeta llena, desconexión durante escritura y corte en tarjeta de prueba; después hacer comprobación de FAT y contar filas completas. `flush()` es void; no se afirma tolerancia a corte ni límite de pérdida garantizado.
- Prueba del auto, cero/escala eléctrica y calibración con instrumentos. La base selecciona `FUENTES_ANALOGICAS`, adecuada para banco configurado, y el cambio no selecciona automáticamente el modo real ni cambia calibraciones.
- Temperaturas en movimiento, tres hilos, pull-up y compatibilidad de cada sensor. La política descarta conversiones parasitarias.
- Display, conectores y consumo global bajo las fuentes de alimentación del vehículo.
