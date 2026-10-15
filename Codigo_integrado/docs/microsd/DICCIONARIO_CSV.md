# Diccionario CSV — schema_version 1

El encabezado contiene **71 columnas**, en el orden siguiente. Se toma directamente del encabezado de producción; esta tabla se valida contra él. Los vacíos son campos sin dato, no ceros. El separador es coma y el decimal es punto. Las precisiones son de presentación; la integración interna conserva su propia precisión.

| # | Columna exacta | Grupo | Descripción / unidad |
|---:|---|---|---|
| 1 | `schema_version` | Identificación | Versión estable del esquema CSV; actualmente 1. |
| 2 | `session_id` | Identificación | Identificador que enlaza todas las partes y el archivo de metadatos. |
| 3 | `registro` | Identificación | Secuencia de captura de 64 bits. Los huecos indican filas omitidas/perdidas. |
| 4 | `timestamp_ms` | Reloj | Milisegundos monotónicos desde el t0 de esta sesión de registro. |
| 5 | `fecha_arg` | Reloj | Fecha local dd/mm/aaaa UTC−03:00; vacía antes de la primera sincronización. |
| 6 | `hora_arg` | Reloj | Hora local HH:MM:SS.mmm UTC−03:00; vacía antes de sincronizar. |
| 7 | `unix_ms` | Reloj | Unix/POSIX UTC en milisegundos; vacío sin referencia de tiempo. |
| 8 | `fuente_hora` | Reloj | GPS, ESTIMADA o SIN_HORA. |
| 9 | `edad_sync_ms` | Reloj | Edad de última referencia UTC monotónica; vacía sin hora. |
| 10 | `segmento_energia` | Energía | Contador de 32 bits; aumenta con reiniciar, no al abrir SD. |
| 11 | `modo_energia` | Energía | Enumeración de Config::MODO; ver metadatos y Configuracion.h. |
| 12 | `sesion_energia_activa` | Energía | 1 acumula, 0 pausada. |
| 13 | `lecturas_energia_disponibles` | Energía | 1 cuando ambas entradas requeridas por el modo son utilizables. |
| 14 | `calibrando_corriente` | Energía | 1 durante calibración de corriente. |
| 15 | `tiempo_integrado_s` | Energía | Segundos realmente integrados por el acumulador; precisión CSV de 0,001 s. |
| 16 | `tension_V` | Electricidad | Tensión de pack usada por Energía, V; vacía si no hay lectura conjunta disponible. |
| 17 | `corriente_A` | Electricidad | Corriente usada por Energía, A; precisión 0,001 A. |
| 18 | `potencia_W` | Electricidad | Potencia del estado de Energía, W; no se vuelve a multiplicar una pareja distinta. |
| 19 | `corriente_media_1s_A` | Resumen | Media de corriente ponderada por tiempo válido de la ventana efectiva, A. |
| 20 | `corriente_max_1s_A` | Resumen | Mayor promedio de corriente observado en la ventana; no es pico analógico. |
| 21 | `potencia_media_1s_W` | Resumen | Media temporal de productos de potencia usados, W. |
| 22 | `ventana_resumen_ms` | Resumen | Duración efectiva del resumen, ms. |
| 23 | `cobertura_resumen_ms` | Resumen | Tiempo que tuvo una medición utilizable; el resto no se rellena con cero. |
| 24 | `ah_consumidos` | Acumulados | Amper-hora descargados del modelo energético, Ah; precisión 0,0001 Ah. |
| 25 | `ah_restantes` | Acumulados | Capacidad nominal menos Ah descargados, Ah. |
| 26 | `wh_consumidos` | Acumulados | Energía descargada estimada, Wh. |
| 27 | `wh_restantes` | Acumulados | Energía nominal restante estimada, Wh. |
| 28 | `soc_pct` | Acumulados | Porcentaje calculado desde Ah por Energía/display; no es el porcentaje antiguo de tensión. |
| 29 | `velocidad_kmh` | Movimiento | Última velocidad PCNT publicada, km/h. |
| 30 | `rpm` | Movimiento | RPM derivadas de PCNT. |
| 31 | `pulsos_ventana` | Movimiento | Pulsos de la última ventana de conteo de un segundo. |
| 32 | `pulsos_total` | Movimiento | Pulsos acumulados de 64 bits. |
| 33 | `pcnt_muestra` | Movimiento | Secuencia de ventana PCNT. |
| 34 | `pcnt_perdidas` | Movimiento | Muestras de ventana PCNT descartadas del buffer. |
| 35 | `temp1_C` | Temperatura | Última lectura válida del sensor 1, °C. |
| 36 | `temp2_C` | Temperatura | Última lectura válida del sensor 2, °C. |
| 37 | `temp_prom_C` | Temperatura | Media espacial de ambas lecturas válidas de la ventana; vacía si falla cualquiera. |
| 38 | `gps_lat` | GPS | Latitud en grados decimales, 6 decimales. |
| 39 | `gps_lon` | GPS | Longitud en grados decimales, 6 decimales. |
| 40 | `gps_vel_kmh` | GPS | Velocidad GPS en km/h. |
| 41 | `gps_rumbo_deg` | GPS | Rumbo GPS en grados; vacío si no es válido, por ejemplo detenido. |
| 42 | `gps_satelites` | GPS | Satélites reportados. |
| 43 | `gps_hdop` | GPS | HDOP reportado. |
| 44 | `adc_tension_V` | Diagnóstico ADC | Voltaje promediado medido en el pin ADC de tensión, V. |
| 45 | `adc_corriente_V` | Diagnóstico ADC | Voltaje promediado del ADC de corriente, V. |
| 46 | `salida_corriente_V` | Diagnóstico ADC | Salida de sensor calculada con el factor configurado, V. |
| 47 | `offset_corriente_V` | Diagnóstico ADC | Offset efectivo de corriente aplicado, V. |
| 48 | `estado_tension` | Calidad | Estado de la entrada de tensión. |
| 49 | `edad_tension_ms` | Calidad | Edad de la adquisición de tensión, ms; vacía si no se vio una. |
| 50 | `estado_corriente` | Calidad | Estado de corriente: puede ser CALIBRANDO. |
| 51 | `edad_corriente_ms` | Calidad | Edad de adquisición de corriente, ms. |
| 52 | `estado_energia` | Calidad | Estado de la actualización del módulo energético. |
| 53 | `edad_energia_ms` | Calidad | Edad de la última actualización energética, ms. |
| 54 | `estado_pcnt` | Calidad | Estado de la última ventana PCNT. |
| 55 | `edad_pcnt_ms` | Calidad | Edad de la ventana PCNT, ms. |
| 56 | `estado_temp1` | Calidad | Validez, conexión y antigüedad del sensor 1. |
| 57 | `edad_temp1_ms` | Calidad | Edad de adquisición del sensor 1, ms. |
| 58 | `estado_temp2` | Calidad | Validez, conexión y antigüedad del sensor 2. |
| 59 | `edad_temp2_ms` | Calidad | Edad de adquisición del sensor 2, ms. |
| 60 | `estado_gps_posicion` | Calidad GPS | Estado/edad de latitud-longitud; sentencia sin ubicación no la renueva. |
| 61 | `edad_gps_posicion_ms` | Calidad GPS | Edad de posición GPS, ms. |
| 62 | `estado_gps_velocidad` | Calidad GPS | Validez/edad de velocidad GPS. |
| 63 | `edad_gps_velocidad_ms` | Calidad GPS | Edad de velocidad GPS, ms. |
| 64 | `estado_gps_rumbo` | Calidad GPS | Validez/edad de rumbo GPS. |
| 65 | `edad_gps_rumbo_ms` | Calidad GPS | Edad de rumbo GPS, ms. |
| 66 | `estado_gps_hdop` | Calidad GPS | Validez/edad del HDOP. |
| 67 | `edad_gps_hdop_ms` | Calidad GPS | Edad de HDOP, ms. |
| 68 | `estado_gps_satelites` | Calidad GPS | Validez/edad de satélites. |
| 69 | `edad_gps_satelites_ms` | Calidad GPS | Edad de satélites, ms. |
| 70 | `registros_perdidos` | Contadores | Pérdidas conocidas al tomar esta captura; consulte también CIERRE/metadatos. |
| 71 | `monitor_omitidos` | Contadores | Filas de monitor Serial omitidas hasta esta captura. |

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

Los estados GPS se calculan independientemente para posición, velocidad, rumbo, HDOP y cantidad de satélites. Edad es tiempo monotónico desde la adquisición original. Para GPS/hora se usa frescura de 3000 ms; otros umbrales están en `src/Registro/ConfigRegistro.h`.

## CSV de ejemplo

`ejemplos/datos_SIMULADOS.csv` contiene filas sintéticas generadas por `formatearCSV` de producción; los nombres/session ID lo indican. Incluye falta de hora, calibración, datos válidos, cero real, sensor 2 desconectado, antigüedad y hueco de secuencia. No contiene mediciones del vehículo. La prueba de stress de longitud trabaja con valores límite no físicos y no exporta ese archivo.
