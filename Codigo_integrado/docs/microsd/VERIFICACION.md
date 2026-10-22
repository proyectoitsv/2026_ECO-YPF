# Verificación de ECO YPF: CSV 70 columnas y panel serial

## Punto de partida

- Base exacta: rama `SD-y-Timestamp`, commit `4934d7ab86693cba92a82bc5bc1248151a12554b`.
- Plataforma compilada: espressif32 6.9.0, ESP32 Dev Module, Arduino-ESP32 2.0.17, GCC Xtensa 8.4.0, esptool 4.5.1; OneWire 2.3.8, DallasTemperature 3.11.0, TinyGPSPlus 1.1.0.
- `pio run` sin acceso a red y con telemetría deshabilitada: **SUCCESS**, firmware binario generado. RAM estática: **29 044 / 327 680 bytes (8,9 %)**. Flash: **404 193 / 1 310 720 bytes (30,8 %)**.
- Los módulos de cálculo, sensores y display recibieron comentarios, pero las fórmulas, constantes, pines y períodos operativos se conservaron. La compilación no reemplaza una prueba sobre el auto.

## Pruebas de software

Reproducir desde `Codigo_integrado` luego de instalar PlatformIO/dependencias:

```sh
pio run
python3 test/ejecutar_pruebas.py
```

Resultado ejecutado: **5 grupos aprobados**. Los binarios de PC se compilaron con C++11, `-Wall -Wextra -Werror`, y se eliminaron al finalizar.

1. Núcleo: fecha gregoriana/bisiestos, Argentina UTC−3, reloj sin hora/GPS/estimado, carrera simulada entre la captura y la nueva sincronización, medias/cobertura, nombres únicos, renombrado y escritura parcial, CSV de 70 campos y regresión energética 48 V, 8,5 A, 3600 s = 408 W, 8,5 Ah, 408 Wh, 50 %.
2. Tareas reales de captura/escritura (`Registro.cpp`) con reloj, colas, Arduino y SD simulados: timestamps conservados, cola llena, ausencia/reintento SD, escritura parcial, cierre/sesión nueva y tarjeta llena. Se añaden fallo inicial y recuperación de metadatos sin pérdida de filas CSV, prueba de fallos aislados de las tres tareas, panel legible, UART sin espacio y mediciones viejas/ausentes. El test recorre el código de producción, con hardware simulado.
3. Regresión del flujo Energía→cola→Display: SOC 50 %, UART, comandos/manual, límites, estados y no repetir valores. Los archivos de display no se modificaron; la prueba usa mocks temporales para FreeRTOS/UART.
4. Parser TinyGPSPlus 1.1.0 y wrapper real con NMEA/checksums, UART simulada: RMC con hora pero sin fix, cambio medianoche, talker GP/GN, RMC con fecha vacía rechazada, sin mezclar fecha antigua, cero detenido sin rumbo, no renovar antigüedad de posición ante GGA sin fix, hora estimada/recuperación y comando ECO diferido sin espera de dos segundos.
5. Tarea DS18B20 real con conversores/FreeRTOS simulados: conversiones paralelas a 12 bits, periodo de 5 s con independencia del movimiento, desconexión individual, promedio vacío y alarma de alimentación parasitaria.

La prueba valida **70 celdas** por fila en `datos_SIMULADOS.csv`; la cabecera mide **1040 bytes**. La fila sintética representativa mide **379 bytes** con ID de 25 caracteres. Un ID como el generado en el firmware tiene 34 caracteres: se estiman **388 bytes/fila** para ese ejemplo. Es una muestra, no un promedio garantizado de carrera. El ensayo de anchos extremos no físicos produjo 1678 bytes y cabe en `MAX_CSV=2048`; coordenadas `double` deliberadamente enormes provocan rechazo completo si el buffer no alcanza, sin escribir filas truncadas.

## RAM y almacenamiento

`RegistroFila` mide **584 bytes** en host y conserva su estructura original; quitar una columna de texto no reduce la cola. Memoria nominal de elementos de colas: 32×584 + 2×584 = **19 856 bytes**, más control/órdenes de FreeRTOS. Las tres pilas conservan 6144 + 10240 + 6144 = **22 528 bytes**. El panel usa un buffer fijo de 1600 bytes y la recuperación de metadatos conserva un encabezado fijo de 2400 bytes; por eso el consumo estático subió. Medir el mínimo real de stack, heap libre, colas y tiempos con `sd estado` en el ESP32.

Estimación orientativa a 1 fila/s, usando 388 bytes de la fila sintética con ID real (solo filas; sin sobrecarga interna FAT):

| Duración | Filas | CSV de datos | Cabecera, una vez |
| --- | ---: | ---: | ---: |
| 1 h | 3 600 | 1 396 800 B (1,33 MiB) | 1 040 B |
| 3 h | 10 800 | 4 190 400 B (4,00 MiB) | 1 040 B |
| 6 h | 21 600 | 8 380 800 B (7,99 MiB) | 1 040 B |

Un sidecar sintético sin fallos midió 1487 bytes, con configuración y cierre. El tamaño real cambia con partes, renombres, eventos y eventuales marcas de recuperación. Añadir margen para FAT, fragmentación y contingencias.

## Lo pendiente en hardware

No se ha conectado ESP32, lector SD, tarjeta, GPS, DS18B20 ni vehículo para esta validación. Probar:

- VCC real del módulo, nivel lógico 3,3 V, CS GPIO5 durante arranque, FAT32 y contactos/cables.
- `sd estado`: heap mínimo, high-water marks de los tres stacks, máximo de cola, máximos de escritura/sync. Repetir con tarjeta lenta y monitor activo.
- Frecuencias efectivas de corrientes/promedios, temperatura 5 s, NMEA/época GPS, captura 1 Hz y jitter de escritura. El objetivo del receptor GPS de 1 Hz requiere medir el modelo/configuración instalada.
- Retiro de tarjeta, tarjeta llena, desconexión durante escritura y corte en tarjeta de prueba; después hacer comprobación de FAT y contar filas completas. `flush()` es void; no se afirma tolerancia a corte ni límite de pérdida garantizado.
- Prueba del auto, cero/escala eléctrica y calibración con instrumentos. La base selecciona `FUENTES_ANALOGICAS`, adecuada para banco configurado, y el cambio no selecciona automáticamente el modo real ni cambia calibraciones.
- Temperaturas en movimiento, tres hilos, pull-up y compatibilidad de cada sensor. La política descarta conversiones parasitarias.
- Display, conectores y consumo global bajo las fuentes de alimentación del vehículo.
