# Plan: integrar 4 sketches en FreeRTOS (PlatformIO / ESP32 WROOM-32)

## Context
Existen 4 sketches independientes (`src/main GPS.cpp`, `main PCNT.cpp`, `main Energia.cpp`, `main Sensor Corriente.cpp`), cada uno con su `setup()/loop()`. Se integran en un único firmware donde cada uno pasa a ser una tarea FreeRTOS (cuerpo de `setup()` antes del `for(;;)`, cuerpo de `loop()` dentro), con periodos adecuados. Debe compilar con `pio run` sin modificar la lógica de los módulos en `Sensores/`.

Este plan está dividido en `README.md` (este archivo) + `etapa-0.md … etapa-7.md`.

## Hallazgos del relevamiento (estado real del proyecto)
- Ningún `main *.cpp` coexiste con otro (4× `setup/loop`, globals duplicados `rpm`, `sensorTemp1/2`, `cuentaPulsos`, `pinSensorInductivo`) → hoy no compila. No existe `src/main.cpp`.
- Los archivos nuevos quedaron mal ubicados respecto de los `#include`: `include/{GPS,Tension,Corriente,Configuracion,SesionEnergia}.h` y `lib/Corriente.cpp` (suelto en `lib/`, PlatformIO lo ignora). Los includes esperan: `src/Configuracion.h` (por `../../Configuracion.h`), `src/Sensores/{GPS,Energia,Corriente,Tension}/…`.
- `src/Sensores/Tension/Tension.h` es la versión VIEJA (sin `lista/valida/ultimaLecturaMs/voltajePinV`) y `Tension.cpp` también (sin `inicializarTension()`); la nueva está solo en `include/Tension.h`. `src/Sensores/Corriente/Corriente.c` está vacío.
- (Resuelto en Etapa 0) `GPS.cpp`, `Energia.h/.cpp` y `Tension.cpp` nuevo fueron aportados y ubicados. `GPS.cpp` usa `TinyGPSPlus` (agregado a `lib_deps`).
- Existe un quinto sketch `src/main tension.cpp` (variante de Energia con `lecturasDisponibles`); también queda excluido por el glob `main?*.cpp`. **Confirmado por el usuario: `src/main tension.cpp` es la versión vigente; `main Energia.cpp` es obsoleto.** Diferencias: usa `lecturasDisponibles` (sin `entradasValidas`), no usa `tension.valida/corriente.valida` ni `MAX_ANTIGUEDAD_LECTURA_MS`; solo exige `lista` (y `!calibrando` en corriente); mensajes de estado distintos.
- PlatformIO NO está instalado en la máquina de desarrollo actual (macOS); el `.pio` existente viene de Windows (COM13). `pio run` debe ejecutarlo el usuario (`! pio run`) o instalarse antes (`pip install platformio`).
- `main Sensor Corriente.cpp` usa API vieja (`obtenerOffset()`, `obtenerPromedioADC()`, `obtenerCorriente()`) y pensada para ESP32-C3; la API vigente es la de `Corriente.h` nuevo. Su función (recalibrar offset por comando) ya existe en `main Energia.cpp` (`calibrar`).
- `main PCNT.cpp` hace `#error` si `ESP_ARDUINO_VERSION_MAJOR != 2`; `platformio.ini` no fija versión de plataforma → riesgo de Arduino-ESP32 3.x.
- Recursos duplicados entre sketches: temperatura (GPIO25/33) en 3 sketches, GPS en 2, entrada GPIO26 usada por ISR clásica (GPS/Energia) y por PCNT. GPIO12 (salida diagnóstico PCNT) es pin de strapping.

## Decisiones de diseño
- `src/main.cpp` nuevo: `setup()` inicializa Serial, mutexes y crea tareas; `loop()` hace `vTaskDelete(NULL)`.
- Originales intactos como referencia, excluidos con `build_src_filter` (ver Etapa 1).
- Velocidad: solo PCNT (descartar la ISR `cuentaPulsos` de GPS/Energia; GPIO26 no puede tener ambas). `Inductivo.cpp` queda sin usar.
- Una sola instancia de `GestorTemperatura` (25 y 33) en su propia tarea; se conserva la regla PCNT "leer temperatura solo mientras no hubo primer pulso" detrás de una constante configurable.
- Un único dueño por periférico: Serial RX (comandos) solo en tarea Energía; Serial TX protegido con mutex; cada módulo con estado `static` interno (Corriente, Tension, GPS) se llama desde UNA sola tarea; el resto lee snapshots.
- Intercambio de datos: `struct DatosCompartidos` en `src/Rtos/Compartidos.h` con snapshots protegidos por mutex (copias cortas).
- Comandos que tocan módulos de otra tarea (`calibrar`) → bandera `volatile`/atómica que consume la tarea dueña.
- Archivos nuevos: `src/Rtos/{Rtos.h,Compartidos.h}`, `src/Tareas/{TareaPCNT,TareaGPS,TareaEnergia,TareaCorriente,TareaTemperatura}.{h,cpp}`, `src/main.cpp`. Flags de habilitación por tarea en `Rtos.h` (no tocar `Configuracion.h`).
- Cada `Tarea*.cpp` encapsula sus globals en namespace anónimo → sin colisiones de nombres.

### Tareas, núcleos y tiempos (tick = 1 ms, `CONFIG_FREERTOS_HZ=1000` por defecto en Arduino-ESP32)
| Tarea | Origen | Core | Prio | Periodo (`vTaskDelayUntil`) | Stack | Notas |
|---|---|---|---|---|---|---|
| Corriente | Sensor Corriente + parte de Energia | 1 | 4 | 5 ms (`INTERVALO_CORRIENTE_MS`) | 3072 | `actualizarSensorCorriente()`; publica `MedidaCorriente`; ejecuta `recalibrar` si hay bandera |
| PCNT | main PCNT | 1 | 3 | 100 ms (vaciar buffer circular; ISR de timer 1 s sin cambios) | 4096 | `configurarPCNT/Timer` dentro de la tarea (la ISR queda en el core 1); publica rpm/km/h |
| Energía | main Energia | 1 | 2 | 20 ms | 4096 | `leerTensionCompleta()`, `SesionEnergia`, comandos Serial, reporte c/ `INTERVALO_REPORTE_MS` |
| GPS | main GPS | 0 | 3 | 10–20 ms (drena Serial2 sin bloquear) | 4096 | `actualizarGPS()`; imprime al `datosGPSActualizados()` |
| Temperatura | GPS/PCNT/Energia | 0 | 1 | 1000 ms | 3072 | `requestTemperatures()` bloquea ~750 ms: aceptable, solo bloquea esta tarea |

Reglas: ninguna tarea sin `vTaskDelay*` (WDT); stacks se validan con `uxTaskGetStackHighWaterMark` en Etapa 6.

## Cómo usar este plan
Cada etapa está en su archivo `etapa-N.md` y puede ejecutarla un agente distinto en otro momento. Orden estricto 0→7. Cada agente: leer este README, ejecutar solo su etapa, validar con `pio run`, tildar abajo y dejar notas.

## Estado
- [x] [Etapa 0 — Prerrequisitos y línea base](etapa-0.md)
- [x] [Etapa 1 — platformio.ini y esqueleto de build](etapa-1.md)
- [x] [Etapa 2 — Infraestructura RTOS](etapa-2.md)
- [x] [Etapa 3 — Tareas Corriente y Energía (Sensor Corriente + main Energia)](etapa-3.md)
- [x] [Etapa 4 — Tarea PCNT (velocidad)](etapa-4.md)
- [x] [Etapa 5 — Tareas GPS y Temperatura](etapa-5.md)
- [x] [Etapa 6 — Validación e integración](etapa-6.md) (PARCIAL: puntos 1-2 hechos; 3-4 requieren hardware)
- [x] [Etapa 7 — Documentación y cierre](etapa-7.md)

## Notas de traspaso
(cada agente agrega aquí lo que hizo, en orden)

**Etapa 0** (hecha, sin compilar: `pio` no disponible): movidos `Configuracion.h→src/`, `GPS.h/.cpp→src/Sensores/GPS/`, `Energia.h/.cpp` y `SesionEnergia.h→src/Sensores/Energia/`, `Corriente.cpp→src/Sensores/Corriente/`, `Tension.h/.cpp nuevos→src/Sensores/Tension/` (reemplazan los viejos). Borrados `include/Corriente.h` (duplicado) y `Corriente.c` vacío. `platformio.ini`: `platform = espressif32@^6.9.0`, `lib_deps += mikalhart/TinyGPSPlus @ ^1.1.0`. `include/` y `lib/` quedan solo con README. Pendiente de verificar al compilar: que `^6.9.0` resuelva y que los módulos compilen juntos.

**Etapa 2** (hecha, sin compilar con `pio`: no disponible; solo chequeo de sintaxis con clang++ y stubs de Arduino/FreeRTOS): creados `src/Rtos/{Rtos.h,Compartidos.h,Compartidos.cpp}`, `src/Tareas/Tarea{Corriente,PCNT,Energia,GPS,Temperatura}.{h,cpp}` (stubs con heartbeat cada 5 s, `vTaskDelayUntil`, expuestos como `bool crearTareaX()`; respetan `HABILITAR_TAREA_*`) y `src/main.cpp` (crea `mtxSerial`, `inicializarCompartidos()`, tareas con `xTaskCreatePinnedToCore`; si falla → log + `ESP.restart()`). Desvíos: el helper de log se llama `logSerial()` (no `logf`, choca con `<math.h>`); `Compartidos` usa un único mutex (`mtxDatos`) con `publicar*/leer*` y bandera `volatile` `solicitarCalibracion()/consumirSolicitudCalibracion()`; `DatosVelocidad`/`DatosTemperatura` definidos en `Compartidos.h`. Cada stub tiene `TODO(etapa)` donde va el setup/loop original. Pendiente: correr `pio run` (gate real) y verificar que `Sensores/*` compilen juntos.

**Gate etapas 0-2** (con `PATH=$PATH:$HOME/.platformio/penv/bin`; `pio` no está en el PATH por defecto): `pio run` OK (espressif32 6.13.0, Arduino-ESP32 2.0.17), RAM 6.7 %, Flash 20.8 %.

**Etapa 3** (hecha, `pio run` OK: RAM 6.8 %, Flash 22.1 %): `TareaCorriente.cpp` real (init con `calibraAlArrancar()`, bucle `consumirSolicitudCalibracion()` → `recalibrarSensorCorriente()`, `actualizarSensorCorriente()`, `publicarCorriente`) y `TareaEnergia.cpp` portado de `main tension.cpp` (comandos, `imprimirEstado/ayuda/prueba patrón` sin cambios de lógica; `calibrar` ahora llama `solicitarCalibracion()`; error de config → imprime y `vTaskSuspend`). Nuevo `src/Tareas/ModoEnergia.h` (helpers `usaTensionADC/usaCorrienteADC/usaACSReal/calibraAlArrancar`). `Rtos.h`: agregado `BloqueoSerial` (RAII sobre `mtxSerial`) y timeout de `logSerial` a 500 ms (un reporte completo ocupa el Serial ~200 ms). Energía lee corriente por snapshot (`leerCorriente()`); la tarea Corriente publica tras recalibrar sin ventana de datos viejos (misma core, mayor prioridad). Sin `delay()` en tareas. Pendiente: probar `calibrar`/`iniciar` en hardware (Etapa 6).

**Etapa 4** (hecha, `pio run` OK: RAM 6.9 %, Flash 23.0 %): `TareaPCNT.cpp` portada de `main PCNT.cpp` sin tocar constantes/ISR/buffer; `configurarPCNT/Timer` + `pcnt_counter_clear/resume` + `timerStart` antes del bucle (core 1); bucle cada 100 ms vacía el buffer, publica `DatosVelocidad` y `primerPulsoRecibido` (vía `leerPrimerPulso/publicarPrimerPulso`), avisa descartadas; se mantienen los `#error` de versión (Arduino-ESP32 2.x) y target. Quitadas tensión y temperatura. `onTimer` verificado en IRAM (`nm`: 0x40081180). Cambios: `DatosVelocidad` ganó `totalPulsos` (uint64); `Rtos.h` nueva constante `PCNT_SALIDA_DIAGNOSTICO` (true por defecto = comportamiento original; false desactiva GPIO12, pin de strapping); si falla `timerBegin` la tarea hace log + `vTaskSuspend` (en vez de bucle con `delay`). La regla "leer temperatura solo mientras no hubo primer pulso" queda para la Etapa 5 (la tarea Temperatura usará `leerPrimerPulso()`). Pendiente: probar con señal real en GPIO26 (Etapa 6).

**Etapa 5** (hecha, `pio run` OK: RAM 7.0 %, Flash 23.6 %): `TareaGPS.cpp` (`inicializarGPS()` antes del bucle, `actualizarGPS()` cada 20 ms, publica `DatosGPS` e imprime como `main GPS.cpp`, con `BloqueoSerial`) y `TareaTemperatura.cpp` (única instancia `GestorTemperatura(25)/(33)`, cada 1 s request+lectura, publica `DatosTemperatura` con `valida` = ambos sensores != -127). Decisión: **`SerialUART1` (13/14) descartado** (estaba comentado como ejemplo, sin dispositivo asociado). Nueva constante `TEMPERATURA_SOLO_SIN_PULSO` (true) en `Rtos.h`: la temperatura se mide solo mientras `!leerPrimerPulso()`. Notas: `inicializarGPS()`/`activarModoAhorro()` (módulo sin modificar) imprimen por `Serial` sin mutex durante el arranque, posible intercalado con otros logs; `datosGPSActualizados()` es una bandera estática del módulo, se llama solo desde esta tarea. Pendiente: probar con GPS/DS18B20 reales (Etapa 6).

**Etapa 6** (PARCIAL, sin placa conectada): (1) `pio run -t clean` + `pio run` → exit 0, sin warnings propios (solo 2 de `OneWire.cpp` en libdeps), RAM 7.0 %, Flash 23.6 % (309 KB), `firmware.bin` generado; no se disparó ningún `#error`; ningún `main *.cpp` original se compila (el filtro funciona; `Inductivo.cpp` sí se compila pero queda sin uso). Nota: `pio run -v` falla en el paso final `firmware.bin` con `TypeError: '_Null' + 'str'` de SCons (bug de la herramienta en modo verbose, no del código); sin `-v` compila bien. (2) Revisión estática OK: cada módulo con estado se llama desde una sola tarea (Corriente→TareaCorriente, Tensión→TareaEnergia, GPS→TareaGPS, Temperatura→instancia local de su tarea); Serial RX solo en Energía; Serial TX siempre por `logSerial`/`BloqueoSerial` salvo `main.cpp` (ruta fatal previa a reiniciar) y los prints internos de `GPS.cpp` en el arranque; `delay()` solo en `main.cpp` (ruta de error) y el `delay(2000)` de `inicializarGPS()` dentro de su tarea; la ISR `onTimer` no usa mutex ni Serial. (3)/(4) PENDIENTES en hardware: `! pio run -t upload` + `pio device monitor` (ajustar `upload_port = COM13` en `platformio.ini` al puerto del Mac, `/dev/cu.usbserial-*`); revisar `stack_libre` en los latidos (margen ≥25 %: ≥1024 B libres en stacks de 4096, ≥768 en 3072), 10+ min sin reset/WDT, pulsos PCNT, GPS/temperatura, comandos `calibrar/iniciar/pausar/estado`, jitter de Corriente. Stacks/prioridades no se tocaron (sin datos).

**Etapa 7** (hecha): cabecera de `src/main.cpp` con el mapa tarea→origen→core→prio→periodo→stack. Los `main *.cpp` originales no fueron modificados en las etapas 2-7 (solo leídos; sus fechas de modificación son anteriores a la Etapa 2). Estado final: firmware integrado compila (`pio run` exit 0, RAM 7.0 %, Flash 23.6 %). **Pendientes abiertos:** validación en hardware (Etapa 6 puntos 3-4: stacks ≥25 % libres, 10+ min sin reset, PCNT/GPS/temperatura/comandos, jitter de Corriente), `upload_port = COM13` a ajustar en el Mac, y decidir si borrar los originales y `Inductivo.cpp` (hoy sin uso) una vez validado.

**Decisiones finales:** velocidad solo por PCNT (GPIO26), ISR clásica descartada; `main tension.cpp` como base de Energía; `SerialUART1` descartado; temperatura única en su tarea con regla `TEMPERATURA_SOLO_SIN_PULSO`; `calibrar` vía bandera; `GPIO12` desactivable con `PCNT_SALIDA_DIAGNOSTICO`; logger `logSerial` (no `logf`).

## Verificación end-to-end
- Gate de cada etapa: `cd <raiz> && pio run` con código de salida 0.
- Final: `pio run -v | grep -i "main "` sin compilar originales; tamaño de firmware reportado; prueba en hardware de Etapa 6 punto 3.

## Riesgos abiertos
- Archivos aún faltantes (Etapa 0 puede bloquear hasta que el usuario los aporte).
- `platform` sin fijar podría traer Arduino-ESP32 3.x (rompe PCNT/timers).
- Glob con espacios en `build_src_filter` (plan B incluido).
- `inicializarGPS()` bloqueante dentro de la tarea GPS: no afecta a otras tareas.

**Etapa 1** (hecha, sin compilar: `pio` no disponible): `platformio.ini` con `build_src_filter = +<*> -<.git/> -<.svn/> -<main?*.cpp>` (verificado con fnmatch: excluye los 5 `main *.cpp`, no `main.cpp`); creado `src/main.cpp` mínimo (Serial + `vTaskDelete(NULL)` en `loop`). Pendiente al compilar: `pio run -v` para confirmar que no se compila ningún `main *.cpp`; si falla el glob, plan B de la etapa.
