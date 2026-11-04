# Diccionario CSV — schema_version 2

El encabezado contiene **70 columnas**, en el orden siguiente. Se toma directamente del encabezado de producción; esta tabla se valida contra él. Los vacíos son campos sin dato, no ceros. El separador es coma y el decimal es punto. Las precisiones son de presentación; la integración interna conserva su propia precisión.

| # | Columna exacta | Grupo | ¿Qué significa, en palabras simples? |
|---:|---|---|---|
| 1 | `schema_version` | Identificación | Versión de esta lista de columnas (2): permite saber cómo leer el archivo. |
| 2 | `session_id` | Identificación | Etiqueta que une todas las partes de una misma sesión. |
| 3 | `registro` | Identificación | Número de esta captura; si salta un número, faltó alguna. |
| 4 | `timestamp_ms` | Reloj | Milisegundos desde el inicio de esta sesión, aunque se pierda el GPS. |
| 5 | `fecha_arg` | Reloj | Fecha argentina de esta captura; vacía si aún no llegó la hora. |
| 6 | `hora_arg` | Reloj | Hora argentina de esta captura; vacía antes de recibir una hora válida. |
| 7 | `unix_ms` | Reloj | El mismo instante en milisegundos del reloj mundial UTC; vacío sin hora. |
| 8 | `fuente_hora` | Reloj | Dice si la hora llegó del GPS, fue estimada o todavía no existe. |
| 9 | `edad_sync_ms` | Reloj | Tiempo desde la última hora informada por GPS, en milisegundos. |
| 10 | `segmento_energia` | Energía | Etapa de energía: aumenta al enviar `reiniciar`. |
| 11 | `modo_energia` | Energía | Modo elegido para las mediciones, sea real o de simulación. |
| 12 | `sesion_energia_activa` | Energía | 1 suma consumo; 0 significa energía pausada. |
| 13 | `lecturas_energia_disponibles` | Energía | 1 cuando las entradas para energía están listas; 0 si faltan. |
| 14 | `calibrando_corriente` | Energía | 1 si el sensor de corriente está aprendiendo su cero; 0 si no. |
| 15 | `tiempo_integrado_s` | Energía | Segundos en los que realmente se sumó consumo; no cuenta las pausas. |
| 16 | `tension_V` | Electricidad | Voltaje del conjunto de baterías usado por el cálculo (V). |
| 17 | `corriente_A` | Electricidad | Corriente usada por el cálculo (A); se conserva aunque sea negativa. |
| 18 | `potencia_W` | Electricidad | Potencia ya calculada a partir de la tensión y corriente usadas (W). |
| 19 | `corriente_media_1s_A` | Resumen | Corriente promedio del último tramo que sí tuvo datos útiles (A). |
| 20 | `corriente_max_1s_A` | Resumen | Mayor promedio de corriente observado en el tramo (A), no el pico analógico. |
| 21 | `potencia_media_1s_W` | Resumen | Potencia promedio durante el tiempo que tuvo datos útiles (W). |
| 22 | `ventana_resumen_ms` | Resumen | Duración del último tramo resumido (ms). |
| 23 | `cobertura_resumen_ms` | Resumen | Parte de ese tramo que tuvo datos aprovechables (ms). |
| 24 | `ah_consumidos` | Acumulados | Carga eléctrica que el modelo lleva gastada desde el último reinicio (Ah). |
| 25 | `ah_restantes` | Acumulados | Capacidad nominal que el modelo calcula que queda (Ah). |
| 26 | `wh_consumidos` | Acumulados | Energía que el modelo calcula como gastada (Wh). |
| 27 | `wh_restantes` | Acumulados | Energía nominal que el modelo calcula que queda (Wh). |
| 28 | `soc_pct` | Acumulados | Porcentaje de batería calculado con Ah y enviado al display. |
| 29 | `velocidad_kmh` | Movimiento | Velocidad calculada con los pulsos de la rueda (km/h). |
| 30 | `rpm` | Movimiento | Vueltas de la rueda por minuto según el contador de pulsos. |
| 31 | `pulsos_ventana` | Movimiento | Pulsos contados en la última ventana de alrededor de un segundo. |
| 32 | `pulsos_total` | Movimiento | Total de pulsos contados desde el inicio del contador. |
| 33 | `pcnt_muestra` | Movimiento | Número de la última ventana del contador de pulsos. |
| 34 | `pcnt_perdidas` | Movimiento | Ventanas de pulsos que se perdieron en el buffer. |
| 35 | `temp1_C` | Temperatura | Temperatura medida por el sensor 1 (°C). |
| 36 | `temp2_C` | Temperatura | Temperatura medida por el sensor 2 (°C). |
| 37 | `temp_prom_C` | Temperatura | Promedio de ambas temperaturas; vacío si falla una. |
| 38 | `gps_lat` | GPS | Posición de norte a sur que informa GPS (latitud, grados). |
| 39 | `gps_lon` | GPS | Posición de este a oeste que informa GPS (longitud, grados). |
| 40 | `gps_vel_kmh` | GPS | Velocidad que informa GPS (km/h), diferente de la velocidad de la rueda. |
| 41 | `gps_rumbo_deg` | GPS | Dirección en la que se mueve el vehículo según GPS (grados). |
| 42 | `gps_satelites` | GPS | Cantidad de satélites que informa GPS. |
| 43 | `gps_hdop` | GPS | HDOP: cómo están ubicados los satélites; más bajo suele ser mejor. |
| 44 | `adc_tension_V` | Diagnóstico ADC | Voltaje leído en el pin de tensión del ESP32 antes de convertirlo al del pack (V). |
| 45 | `adc_corriente_V` | Diagnóstico ADC | Voltaje leído en el pin de corriente del ESP32 (V). |
| 46 | `salida_corriente_V` | Diagnóstico ADC | Voltaje de salida del sensor de corriente, aplicando el factor configurado (V). |
| 47 | `offset_corriente_V` | Diagnóstico ADC | Voltaje que se considera corriente cero (V). |
| 48 | `estado_tension` | Calidad | Dice si la tensión es nueva, vieja, simulada o si falta una lectura. |
| 49 | `edad_tension_ms` | Calidad | Tiempo desde la última lectura de tensión (ms); vacío si nunca hubo una. |
| 50 | `estado_corriente` | Calidad | Estado de corriente: también avisa cuando se está calibrando. |
| 51 | `edad_corriente_ms` | Calidad | Tiempo desde la última lectura de corriente (ms). |
| 52 | `estado_energia` | Calidad | Estado de la actualización energética; no indica por sí solo la edad de cada sensor. |
| 53 | `edad_energia_ms` | Calidad | Tiempo desde la última actualización energética (ms). |
| 54 | `estado_pcnt` | Calidad | Estado de la última medición de pulsos de la rueda. |
| 55 | `edad_pcnt_ms` | Calidad | Tiempo desde esa medición de pulsos (ms). |
| 56 | `estado_temp1` | Calidad | Estado de temperatura 1: nueva, vieja o con problema. |
| 57 | `edad_temp1_ms` | Calidad | Tiempo desde la lectura de temperatura 1 (ms). |
| 58 | `estado_temp2` | Calidad | Estado de temperatura 2: nueva, vieja o con problema. |
| 59 | `edad_temp2_ms` | Calidad | Tiempo desde la lectura de temperatura 2 (ms). |
| 60 | `estado_gps_posicion` | Calidad GPS | Estado de posición GPS: válida, vieja o sin ubicación. |
| 61 | `edad_gps_posicion_ms` | Calidad GPS | Tiempo desde la última posición GPS (ms). |
| 62 | `estado_gps_velocidad` | Calidad GPS | Estado de la velocidad que informó GPS. |
| 63 | `edad_gps_velocidad_ms` | Calidad GPS | Tiempo desde esa velocidad GPS (ms). |
| 64 | `estado_gps_rumbo` | Calidad GPS | Estado de la dirección que informó GPS. |
| 65 | `edad_gps_rumbo_ms` | Calidad GPS | Tiempo desde esa dirección GPS (ms). |
| 66 | `estado_gps_hdop` | Calidad GPS | Estado del dato HDOP. |
| 67 | `edad_gps_hdop_ms` | Calidad GPS | Tiempo desde el último HDOP (ms). |
| 68 | `estado_gps_satelites` | Calidad GPS | Estado del dato de satélites. |
| 69 | `edad_gps_satelites_ms` | Calidad GPS | Tiempo desde el último dato de satélites (ms). |
| 70 | `registros_perdidos` | Contadores | Capturas que se sabía que faltaban al crear esta fila; puede haber otras posteriores. |

## Estados

- `OK`: válido y dentro del umbral de frescura configurado.
- `ANTIGUA`: dato válido conservado, pero excede su umbral; su valor no se presenta como recién adquirido.
- `SIN_DATO`: todavía no se vio una lectura; edad vacía.
- `INVALIDA`: la fuente publicó un valor inválido.
- `CALIBRANDO`: la corriente está calibrando; su celda analógica no se presenta como medida utilizable.
- `NO_DISPONIBLE`: el estado energético no publicó una combinación utilizable.
- `DESCONECTADO`: el sensor térmico dio lectura de desconexión/rango inválido.
- `ERROR_ALIMENTACION`: el DS18B20 informa modo parasitario; comprobar alimentación externa de tres hilos.
- `SIN_FIX`: posición sin fix; `SIN_RUMBO`: rumbo no fiable, incluido vehículo detenido.
- `SIMULADA`: magnitud producida por el modo de referencia/configuración no ADC.

El contador `monitor_omitidos` sigue en `sd estado` y en el cierre de metadatos: no ocupa una columna en cada fila. La corriente negativa se conserva como dato leído.

Los estados GPS se calculan independientemente para posición, velocidad, rumbo, HDOP y cantidad de satélites. Edad es tiempo monotónico desde la adquisición original. Para GPS/hora se usa frescura de 3000 ms; otros umbrales están en `src/Registro/ConfigRegistro.h`.

## CSV de ejemplo

`ejemplos/datos_SIMULADOS.csv` contiene filas sintéticas generadas por `formatearCSV` de producción; los nombres/session ID lo indican. Incluye falta de hora, calibración, datos válidos, cero real, sensor 2 desconectado, antigüedad y hueco de secuencia. No contiene mediciones del vehículo. La prueba de stress de longitud trabaja con valores límite no físicos y no exporta ese archivo.
