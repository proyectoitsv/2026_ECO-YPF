# ECO YPF — microSD, panel serial y timestamp, versión microsd-2

Proyecto completo para ESP32-WROOM-32 / PlatformIO. Esta versión parte de `SD-y-Timestamp`, commit `4934d7ab86693cba92a82bc5bc1248151a12554b`; la integración original microSD había partido de `Display-Sparkfun` (`e4ca276507141481e6e8dc57f8706e6747a8eb69`). Esta actualización no se publicó ni modificó ninguna rama remota.

El registro comienza automáticamente, aunque no haya tarjeta, hora GPS ni adquisición eléctrica lista. Captura los últimos datos publicados una vez por segundo y los entrega por valor a una cola. Otra tarea atiende la tarjeta. El cálculo energético sigue arrancando **pausado** y se activa con `iniciar`.

## Instalación en Visual Studio Code

1. Conservá una copia de tu carpeta actual. Extraé el ZIP en una carpeta aparte.
2. Abrí **la carpeta `Codigo_integrado` del ZIP** en VS Code con PlatformIO. Es la forma recomendada de probar esta integración y revisar diferencias sin sobrescribir tu trabajo.
3. Si preferís integrarlo a tu copia existente, compará la carpeta completa: hay archivos nuevos y varios modificados. Copiá conservando las rutas y revisá los cambios en Source Control. No alcanza con reemplazar un `.cpp`. `CAMBIOS.md` resume los archivos modificados y la comparación con la base.
4. Revisá `src/Configuracion.h`. Se conserva **`FUENTES_ANALOGICAS`**, las escalas, los pines y las calibraciones de la base. Este modo es para banco; usar sensores reales requiere seleccionar y verificar la configuración correspondiente a tu montaje. No se cambió automáticamente a `SENSORES_REALES`.
5. Elegí el puerto de tu ESP32: `platformio.ini` conserva `COM7` de la base. Ajustalo o indicá `pio run -t upload --upload-port TU_PUERTO`.
6. Ejecutá Build y luego Upload. Abrí el monitor a **115200 baudios**, con fin de línea Enter/CR/LF.

Versiones fijadas: `espressif32@6.9.0`, Arduino-ESP32 **2.0.17**, OneWire **2.3.8**, DallasTemperature **3.11.0** y TinyGPSPlus **1.1.0**. SD, SPI y Preferences pertenecen al framework. Se conserva Arduino 2.x porque PCNT usa las API de timers de esa generación. No actualizar a Arduino 3.x para esta prueba.

## Conexiones y preparación

| Módulo / señal | GPIO ESP32 |
| --- | ---: |
| SD CS | 5 |
| SD SCK | 18 |
| SD MISO | 19 |
| SD MOSI | 23 |
| Tensión / corriente ADC | 34 / 35 |
| PCNT inductivo | 26 |
| DS18B20 1 / 2 | 25 / 33 |
| GPS RX ESP32 / TX ESP32 | 16 / 17 |
| Display TX ESP32 / RX reservado | 13 / 14 |

No se encontraron conflictos entre estos pines. Se usa VSPI explícito a **1 MHz**, configurable en `src/Registro/ConfigRegistro.h`. GPIO5 es un pin de strapping: el lector y su CS deben permitir el arranque correcto del ESP32. Comprobar el montaje real antes de energizarlo.

**Todavía falta identificar el modelo y la alimentación del lector.** Verificá VCC en la documentación de ese módulo y que las señales que llegan al ESP32 sean compatibles con **3,3 V**. Este código no determina si el lector requiere 3,3 V o 5 V en su entrada de alimentación. Usá masa común y cableado SPI corto.

Prepará la tarjeta como **FAT32** en la computadora. El firmware nunca formatea ni borra archivos al fallar el montaje. En el framework compilado se verificó `CONFIG_FATFS_LFN_STACK=1`, `CONFIG_FATFS_MAX_LFN=255`, `FF_USE_LFN=2` y `FF_FS_EXFAT=0`: hay nombres largos y no hay soporte exFAT. Se incluye una alternativa 8.3 para otra compilación que deshabilite LFN.

Los DS18B20 se operan con alimentación externa de **tres hilos**, su resistencia de pull-up y niveles adecuados al ESP32. Si DallasTemperature detecta alimentación parasitaria, ese bus se marca `ERROR_ALIMENTACION` y no se ejecuta una conversión asíncrona que requiera strong pull-up. Verificá físicamente VDD, masa y datos de ambos sensores; la detección de software no reemplaza esa comprobación.

## Comandos y sesiones

| Comando | Efecto |
| --- | --- |
| `iniciar` / `i` | Comienza/reanuda Ah, Wh y tiempo integrado, cuando hay mediciones disponibles. |
| `pausar` / `p` | Congela acumulados; continúa adquisición, display y registro. |
| `reiniciar` / `r` | Pone acumulados en cero, queda pausado e incrementa `segmento_energia`. No reinicia el timestamp SD. |
| `estado` | Reporte energético existente. |
| `test` | Caso patrón virtual independiente, disponible con energía pausada. |
| `calibrar` | Conserva las condiciones previas: ACS real, sin carga y energía pausada. |
| `display NUMERO` / `display auto` | Prueba manual / SOC energético automático. El número manual no se registra como SOC. |
| `sd estado` | Archivo, estado, pérdidas, ocupación máxima, heap, stacks y tiempos de escritura/sincronización. |
| `sd cerrar` | Detiene capturas, drena lo pendiente y solicita sincronización/cierre desde la tarea SD. Esperar el mensaje de cierre. |
| `sd nueva` | Cierra la anterior e inicia otra sesión de registro con timestamp relativo en cero. Conserva Ah/Wh y estado de energía. |

El único lector de Serial RX sigue siendo Energía. Las órdenes SD pasan por una cola y se confirman de forma asíncrona. Si no hay archivo al cerrar, los registros pendientes que no pueden guardarse se descartan y contabilizan; no se declara que fueron escritos.

`timestamp_ms` usa `(esp_timer_get_time() - t0_us) / 1000`, con 64 bits. `t0_us` se fija antes de lanzar la adquisición; la primera fila usa el instante real de captura y puede estar unos milisegundos después de cero. No depende de `iniciar`, de filas escritas ni del timer PCNT. `tiempo_integrado_s` pertenece al acumulador y puede estar detenido mientras el timestamp avanza.

Al reiniciar el ESP32, el modelo energético arranca con consumo cero y SOC nominal de 100 %. **No recupera la carga real anterior**: al analizar una carrera, interpreten los Ah, Wh y SOC desde el comienzo de esa sesión o del último comando `reiniciar`. `sd nueva` solo inicia otro archivo y reloj relativo, sin reiniciar esos acumulados. La corriente negativa se registra con su signo; el modelo actual no la utiliza para devolver carga a los acumulados.

`session_id` combina MAC, contador de arranques NVS y número de sesión local. Si no puede persistir el contador, se usa un identificador aleatorio de arranque. Los nombres se comprueban además contra los archivos existentes. No borrar NVS durante pruebas que deban conservar una continuidad del contador de arranques.

## Reloj y archivos

Antes de recibir UTC válido: `fuente_hora=SIN_HORA`; fecha, hora, Unix y edad de sincronización quedan vacíos. La hora se valida desde fecha y hora de una misma RMC reciente, comprobando campos no vacíos; funciona incluso sin posición válida. Una GGA no se combina con una fecha RMC anterior al cruzar medianoche.

Después de sincronizar se interpola UTC con el reloj monotónico. `GPS` significa referencia reciente, hasta **3000 ms**, incluso entre mensajes; después se indica `ESTIMADA`. `edad_sync_ms` continúa creciendo. `unix_ms` siempre es POSIX UTC; solamente fecha/hora argentinas aplican UTC−03:00. La recepción NMEA introduce latencia: los milisegundos de presentación no prometen exactitud UTC de 1 ms ni sincronización PPS.

Ejemplos de nombres:

- Sin hora: `Prueba_SIN_HORA_000001_P000.csv`.
- Fechado: `Prueba_2026-10-14_14-00-35_001_P000.csv`.
- Recuperación: otra parte `P001`, manteniendo `session_id` y reloj relativo.

El nombre fechado estima el inicio a partir del enlace UTC/monotónico. Al llegar la hora se sincroniza, cierra, renombra y reabre en append. Si el renombrado falla se intenta reabrir la ruta original; si queda incierta se recupera en otra parte. No se sobrescriben las filas sin hora para rellenarlas después.

El archivo `Meta_<session_id>.txt` relaciona todas las partes, configuración, versión, causa de reinicio, enlace temporal y eventos `SYNC`, `RENOMBRE` y `CIERRE`. Una `SYNC` se guarda al primer enlace o cuando el ajuste supera ±250 ms; las sincronizaciones normales también actualizan el reloj. Con 8.3 se emplea el basename CSV más `.TXT`. Los ceros de referencia en metadatos sin hora están identificados por `inicio_absoluto_estimado=0`; no representan una fecha real. `calibracion_corriente_modo`: 0 offset fijo, 1 automática en cero, aplicada solamente en los modos ACS real existentes. El offset efectivo se conserva por fila en `offset_corriente_V`. Si al abrir metadatos ocurre un fallo temporal, se reintentan por separado cada 10 s sin interrumpir las filas CSV. `RECUPERACION_META` deja visible que pudo haber eventos anteriores no guardados; `fallos_meta` informa los intentos fallidos y `eventos_hora_perdidos_cola` cuenta solamente desbordes de la cola de eventos.

## Datos, errores y límites

El esquema **2 tiene 70 columnas**: conserva las 70 primeras columnas anteriores y quita solamente `monitor_omitidos` de cada fila. Conserva las cuatro columnas ADC y los indicadores de calidad. El separador es coma, el decimal es punto y hay una cabecera por archivo. Consultá `docs/microsd/DICCIONARIO_CSV.md`, que explica cada valor en palabras simples. Un cero válido se escribe como cero; un dato desconocido/invalidado queda vacío y lleva estado. Una última lectura válida puede repetirse con edad original y estado `ANTIGUA`. Las temperaturas se conservan individualmente; si una falla no se calcula el promedio conjunto.

El snapshot copia los últimos datos disponibles bajo un mutex breve. Los sensores no se muestrean físicamente al mismo tiempo. Para electricidad se copia la misma combinación utilizada por Energía, junto con marcas individuales de adquisición. Los resúmenes usan intervalos reales, medias ponderadas por tiempo y cobertura; el máximo es el mayor **promedio de corriente observado**, no el pico analógico instantáneo. Se calculan también durante pausa, sin alterar la integración de Ah/Wh.

La cola SD guarda **32 tramas**. A 1 Hz amortigua aproximadamente 32 segundos de interrupción si comenzó vacía; no conserva ausencias indefinidas. Si se llena se descarta la **captura nueva**, sin esperar a SD, y avanzan secuencia y pérdidas. Tampoco se fabrican filas de recuperación para segundos perdidos. Las marcas de captura permanecen intactas al escribirlas más tarde.

La tarjeta se reintenta cada **10 segundos**, sin formatear ni reiniciar por un fallo SD. Si se llena una tarjeta ya montada, el archivo confirmado se conserva abierto, se frena el vaciado de la cola y se reintenta a los 10 segundos. La fila que detectó el límite se cuenta como perdida; si el usuario cierra sin liberar espacio, los pendientes también se contabilizan. En falta de espacio inicial no se crea una sucesión de archivos vacíos. Una escritura parcial conserva la parte incierta y abre otra, sin concatenar una fila nueva al fragmento anterior. Una parte incierta puede terminar en una línea incompleta: al analizarla descartá esa última línea y correlacioná las demás partes por sesión/registro. No asumir que toda fila aceptada por `write` ya sobrevivirá a un corte.

Se solicita `flush()` cada 2 segundos y al cerrar/renombrar. Esa API devuelve `void`; se revisan longitud de `write` y errores disponibles, pero **no se garantiza una pérdida máxima de 2 segundos ni persistencia física**. Un corte puede afectar datos o FAT. Los metadatos son otro archivo y también pueden fallar; se informa el error aunque el CSV pueda continuar. `registros_perdidos` refleja lo conocido al capturar; pérdidas posteriores aparecen en capturas siguientes, en `sd estado` y en el evento final `CIERRE` cuando puede escribirse.

El monitor consume una copia de la misma captura a 1 Hz, con cola de 2 y TX de 4096 bytes. Presenta un panel con nombres y unidades; los CSV de la tarjeta siguen siendo los datos para analizar la carrera. Los datos viejos se marcan `ANTIGUA`; si falta una lectura, se muestra su estado en lugar de un cero supuesto. Si no hay espacio UART/mutex se omite el panel y se cuenta `monitor_omitidos` en `sd estado` y en el cierre de metadatos. Se mantiene la ayuda y todos los comandos. La línea `Último error registrado` puede describir un fallo ya recuperado. Si no arranca el monitor, la captura y el CSV continúan; si falta la tarea de captura o de SD, se informa que el registrador no está disponible. Heartbeats y reportes extensos están apagados.

## Frecuencias y FreeRTOS

| Función | Período / comportamiento final |
| --- | --- |
| Corriente | Tarea 5 ms, 15 muestras: promedio nuevo nominal cada 75 ms. |
| Calibración de corriente | Solo en los modos ACS reales que la solicitan: estabilización de 500 ms y 500 muestras atendidas cada 5 ms; primer promedio utilizable cercano a 3,07 s en la simulación de tarea sin retrasos. |
| Tensión | Configurado 50 ms, 5 muestras; atendida por Energía cada 20 ms: puede resultar ~60 ms / ~300 ms por promedio sin otros retrasos. |
| Energía | 20 ms; integración con el tiempo real anterior, arranque pausado. |
| PCNT | Conteo continuo, ventana 1000 ms; tarea atiende buffer cada 100 ms. |
| GPS UART | Drenado cada 20 ms, 9600 baudios. ECO diferido 2 s sin suspender recepción. |
| Posiciones GPS | Objetivo 1 Hz según receptor; frecuencia física pendiente de medir. No son 50 posiciones/s. |
| DS18B20 | Inicio cada 5000 ms; ambas conversiones en paralelo, 12 bits y una espera de 750 ms que cede CPU. |
| Publicación al display | Cada 500 ms; cola de último estado. Visibilidad 2 s / 3 s, intacta. |
| Captura CSV / monitor | Inicial y objetivos cada 1000 ms; tiempos reales, huecos contabilizados. |
| Escritora SD | Servicio cada 20 ms; una fila disponible por iteración. Sync objetivo 2000 ms. |
| Reintento SD | 10000 ms. |

Se agregan `CapturaCSV` (core 0, prioridad 2, stack 6144 bytes), `EscritoraSD` (core 0, prioridad 1, 10240 bytes) y `MonitorCSV` (core 0, prioridad 1, 6144 bytes). La SD tiene un único dueño. GPS mantiene prioridad 3 en core 0; Corriente/PCNT/Energía mantienen core 1 y prioridades 4/3/2. Se habilita Temperatura con prioridad 1 en core 0, conservando stack 3072. Display conserva tarea, driver, UART y ciclo originales. No se reconfiguró el timer PCNT.

`HABILITAR_SENSORES_AUXILIARES` en `Configuracion.h` participa en la verificación de conflictos de pines. Las opciones `HABILITAR_TAREA_*` de `Rtos/Rtos.h` controlan por separado qué tareas se crean; cambiar solo la primera opción no detiene GPS, temperatura ni PCNT.

La integración microSD original añadió tareas y estructuras compartidas. Esta actualización conserva sus núcleos, prioridades, tamaños de stack, colas y tiempos. En los módulos de cálculo `Energia`, `SesionEnergia`, `Corriente` y `Tension`, y en el display, se añadieron únicamente comentarios: se comparó el código operativo con el commit de referencia. PCNT conserva 8 pulsos/vuelta, diámetro de rueda de 0,49 m y la relación mecánica implícita 1:1: comprobar que coincide con el vehículo, sin corregirla silenciosamente.

## Prueba inicial y recuperación

1. Verificá conexiones/alimentación, FAT32 y modo energético de banco antes de cargar. Compilá el proyecto completo.
2. Arrancá sin recepción GPS: comprobá timestamp creciente, estado `SIN_HORA`, energía pausada y archivo provisional. Probá también sin tarjeta: las mediciones deben continuar y `sd estado` debe mostrar reintentos/pérdidas.
3. Con tarjeta presente y espacio, enviá `sd estado`. Comprobá archivo abierto, escritos crecientes y cola sin crecimiento sostenido. Dejá transcurrir al menos un minuto.
4. Obtené hora GPS: verificá fecha local UTC−3, `unix_ms` UTC, renombrado y evento `SYNC`. Interrumpí y restablecé recepción: `ESTIMADA` al superar 3 s, sin reinicio del tiempo relativo.
5. Con lecturas disponibles enviá `iniciar`, luego `pausar`, reanudá y finalmente `reiniciar`. El timestamp continúa; acumulados/tiempo integrado se congelan en pausa y `segmento_energia` cambia al reiniciar. `sd nueva` sí reinicia el timestamp SD, conservando energía.
6. Verificá ambos DS18B20 en movimiento durante varios ciclos. Desconectá uno en una prueba controlada: su celda y el promedio deben quedar vacíos, conservando el otro. Confirmá el ciclo de display 2 s / 3 s y los comandos manual/auto.
7. Enviá `sd cerrar` y esperá la confirmación antes de retirar la tarjeta. Si hubo error de cierre, conservá los archivos para inspeccionarlos; la confirmación indica cierre lógico y solicitud de flush, no garantía física.
8. Copiá CSV y metadatos juntos al PC, importá con coma/punto, revisá las 70 columnas, edades, cobertura, pérdidas y partes. Si apareció `RECUPERACION_META`, revisá qué eventos anteriores pudieron faltar. No borres una parte incierta antes de recuperar sus filas completas.

Medí en hardware `max_write_us`, `max_sync_us`, máximo de cola, heap y stack libre de `sd estado`. Para períodos efectivos calculá diferencias de `timestamp_ms`; para fuentes discretas reconstruí el instante relativo como timestamp menos su edad y compará solamente cuando cambia. Para verificar si el GPS supera 1 Hz, capturá las RMC/GGA del receptor mediante un adaptador UART compatible de lectura pasiva o una compilación de diagnóstico: el CSV a 1 Hz no distingue todas las actualizaciones más rápidas. Identificá mensajes de una misma época para no contar RMC y GGA como dos posiciones distintas.

Ensayá tarjeta lenta, desconectada, llena y un corte de alimentación **en una tarjeta de prueba**. Después verificá tanto el sistema FAT como las filas completas y la última parte. La compilación y simulaciones no reemplazan estos ensayos ni prueban latencias, alimentación, ruido, deriva UTC o margen real de stack.

## Verificación y archivos incluidos

- `docs/microsd/VERIFICACION.md`: compilación, cinco grupos de pruebas, memoria, espacio y límites de lo validado.
- `docs/microsd/DICCIONARIO_CSV.md`: columnas exactas, precisiones y estados.
- `docs/microsd/ejemplos/`: datos y metadatos **simulados**; no son mediciones de vehículo.
- `test/ejecutar_pruebas.py`: pruebas reproducibles en PC con Python 3 y g++, después de `pio run`/`pio pkg install`.

Ejecutar desde `Codigo_integrado`:

```sh
pio run
python3 test/ejecutar_pruebas.py
```

El caso patrón 48 V × 8,5 A durante 3600 s verifica 408 W, 8,5 Ah, 408 Wh y SOC 50 % del modelo nominal 17 Ah / 48 V. No es una medición de la capacidad real de las baterías.

Referencias contrastadas con el código instalado: [esp_timer ESP-IDF 4.4](https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/api-reference/system/esp_timer.html), [SPI Arduino-ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/spi.html), [SD Arduino-ESP32 2.0.17](https://github.com/espressif/arduino-esp32/tree/2.0.17/libraries/SD), [DS18B20](https://www.analog.com/media/en/technical-documentation/data-sheets/ds18b20.pdf).
