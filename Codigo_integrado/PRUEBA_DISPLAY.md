# Display SOC — ECO YPF con FreeRTOS

Integración basada en la rama `FreeRTOS`, commit `bc9f9354d588adbd6e76db94d74b2518ca29c0cc`.

## Conexión

| ESP32-WROOM-32 | COM-11441 |
| --- | --- |
| GPIO13 / D13, TX de UART1 | RX del display |
| GND | GND |
| 3V3 | VCC |

UART1 se inicializa a **9600 baudios, 8N1**, con RX en GPIO14 / D14.
GPIO14 queda reservado, pero no requiere un cable para esta comunicación de salida.
El GPS conserva Serial2 en GPIO16/17. El monitor USB conserva Serial a 115200 baudios.

## Funcionamiento

1. `setup()` crea una cola de un elemento antes de iniciar las tareas.
2. `TareaEnergia` publica en esa cola el SOC que ya calculó, cada 500 ms.
3. `TareaDisplay` espera con `xQueueReceive()` y envía el número por UART1.

La cola copia una estructura y conserva el estado más reciente mediante
`xQueueOverwrite()`: Energía no espera espacio ni transmite bytes al display.
No se acumulan porcentajes antiguos pendientes. La tarea del display tiene
prioridad 1, núcleo 0 y stack de 3072 bytes.

El SOC se muestra como un entero redondeado entre 0 y 100. Por ejemplo,
73.2 % se muestra como `  73` y 73.6 % como `  74`. El módulo no recalcula
SOC desde tensión. Solo actualiza los dígitos cuando cambia el entero mostrado.
Los cuatro dígitos muestran el número: no aparece un símbolo `%`.

Antes del primer promedio disponible y durante una calibración del ACS,
muestra `----`. El firmware sigue arrancando en pausa. Luego de tener
lecturas, el SOC inicial será 100; enviar `iniciar` permite acumular consumo.
`pausar` mantiene el SOC, `iniciar` continúa y `reiniciar` devuelve los
acumulados a cero. No hay persistencia frente a reinicios.

## Primera prueba con el display

Compilar y cargar el proyecto `Codigo_integrado` con PlatformIO, ambiente `esp32dev`.
Abrir el monitor a 115200 baudios y enviar comandos terminados en Enter:

```text
display 1234
display 73
display auto
estado
iniciar
```

- `display 1234`: muestra 1234 para probar UART y los cuatro dígitos.
- `display 73`: muestra 73 en modo manual.
- `display auto`: vuelve al SOC actual publicado por Energía.
- `estado`: informa el modo del display además de las mediciones.
- `iniciar`: comienza la integración real; el display acompaña el SOC.

La prueba manual admite enteros entre -999 y 9999. No cambia el consumo
ni pausa automáticamente Energía. El modo manual se mantiene hasta enviar
`display auto`, aunque sigan llegando publicaciones de Energía.

El comando `test` conserva la prueba matemática separada: una hora virtual
a 48 V y 8.5 A produce 50 % de SOC, pero no altera la sesión ni fuerza 50
en el display del vehículo. Para probar los dígitos de 50 usar `display 50`.

## Parámetros editables

En `src/Configuracion.h`: `PIN_TX_DISPLAY`, `PIN_RX_DISPLAY`, `BAUDIOS_DISPLAY`,
`BRILLO_DISPLAY`, `ESTABILIZACION_DISPLAY_MS` e `INTERVALO_ENVIO_SOC_MS`.
En `src/Rtos/Rtos.h`: `HABILITAR_TAREA_DISPLAY`, `PRIO_DISPLAY` y `STACK_DISPLAY`.
Recompilar y cargar después de cambiar constantes.

Se conservan los parámetros de sensores del commit base, incluida la
sensibilidad de corriente **0.033 V/A**. Esta integración no corrige el ADC.
Con esa sensibilidad, una variación efectiva de 0.340 V corresponde a
10.30 A, y 8.5 A requieren una variación efectiva de 0.2805 V. Comparar
el display con el SOC del monitor, no con el antiguo porcentaje de Tensión.

## Archivos para incorporar

Reemplazar únicamente:

- `src/Configuracion.h`
- `src/Rtos/Rtos.h`
- `src/Tareas/TareaEnergia.cpp`
- `src/main.cpp`

Agregar `src/Display/`, `src/Rtos/ColaDisplay.h/.cpp`,
`src/Tareas/TareaDisplay.h/.cpp`, este documento y la prueba de `test/`.
El ZIP de cambios incluye solo esos archivos, dentro de `Codigo_integrado`.
Copiar su contenido dentro de la carpeta del mismo nombre del repositorio.
Conservar el `platformio.ini` actual: ya contiene plataforma 6.x, el filtro
que excluye los sketches antiguos y COM4. No incorporar otros `main` de prueba.

## Verificación realizada

- Compilación del firmware con PlatformIO, ESP32 clásico y Arduino-ESP32 2.0.17.
- Prueba en computadora del productor y consumidor reales con UART/cola
  simulados: SOC 50 %, UART TX13/RX14, modo manual/automático, último estado,
  redondeo, límites, espera inicial y ausencia de reenvíos del mismo número.
- La prueba en computadora no reproduce el planificador real ni verifica
  el cableado. La comprobación física del display se hace en el ESP32.

Prueba opcional en Linux con g++, desde `Codigo_integrado`:

```bash
g++ -std=c++11 -Wall -Wextra -Werror -Wno-sign-compare -I test/display_host \
  test/prueba_display_host.cpp src/Rtos/ColaDisplay.cpp \
  src/Display/Display_COM11441.cpp src/Tareas/TareaDisplay.cpp \
  src/Sensores/Energia/Energia.cpp -o /tmp/prueba_display_host
/tmp/prueba_display_host
```

La prueba queda fuera de `src` y no se carga en el ESP32.
