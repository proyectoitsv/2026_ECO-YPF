# Energía V2.1: fuentes sin rechazo de lecturas ni promedio recortado

Esta actualización está basada en la rama prueba_energía, commit 6537e597c4536a66191f5c15f758e6ecf8b35119. El ZIP trae solo los archivos que hay que reemplazar en Codigo_integrado; no trae platformio.ini, GPS, Inductivo ni Temperatura. Conservá tu puerto COM4.

Cambios pedidos: se quitaron entradaValida(), el rechazo del grupo de muestras y la suspensión por rango/saturación/antigüedad. Se quitaron mediaRecortada(), el descarte de extremos, el escalonado y el recorte de tensión. Tensión y corriente ahora promedian todas las muestras y convierten sus valores linealmente. El monitor ya no informa FUERA DE ESCALA ni NO VALIDAS.

Se mantienen los comandos, el modo de fuentes con offset fijo, la calibración no bloqueante del ACS real y el cálculo de energía con tiempo real. Solo se espera el primer promedio o la finalización de una calibración. SOC continúa limitado entre 0 y 100 %, y Ah/Wh restantes nunca son negativos. La verificación de parámetros al arrancar evita, por ejemplo, una sensibilidad cero; no valida las señales medidas.

## Copiar, compilar y cargar

1. Cambiar a tu rama de prueba antes de copiar.
2. Descomprimir el ZIP. Copiar SU carpeta Codigo_integrado sobre la carpeta Codigo_integrado del clon, fusionando carpetas y aceptando los reemplazos. No crear Codigo_integrado/Codigo_integrado.
3. Abrir Codigo_integrado en VS Code/PlatformIO, verificar el puerto de la placa en platformio.ini (actualmente COM7), y ejecutar Build y Upload para esp32dev.
4. Abrir Serial Monitor a 115200 baudios. Si no se vio el arranque, pulsar EN/RESET.
5. El modo inicial es FUENTES_ANALOGICAS. Arranca PAUSADO: las fuentes se leen pero no se acumula consumo hasta enviar iniciar.
6. Revisar los cambios y hacer tu commit. No hace falta copiar ningún archivo adicional de la entrega anterior.

## Conexión de banco

- ESP32-WROOM-32 alimentado por USB.
- Fuente de tensión equivalente: positivo a GPIO34, señal entre 0,450 y 1,250 V.
- Fuente de corriente equivalente: positivo a GPIO35, referencia 2,500 V para 0 A y 2,840 V para +8,5 A.
- Referenciar las señales al GND del ESP32, verificando que las salidas de las fuentes permitan esa conexión común.
- Medir las tensiones con un multímetro ANTES de conectar los pines. Nunca aplicar los 48–52 V de batería a GPIO34 ni una señal de 5 V a GPIO35.
- Esta versión usa lo que mide el ADC sin comprobar rango o saturación. Por eso una señal mal conectada también produce un resultado y puede alterar los acumulados. El cambio de software no amplía el rango físico ni mejora la precisión del ADC.

No se conectan baterías ni ACS758 para esta prueba: las fuentes imitan sus señales de entrada. No existe una secuencia temporal programada ni una duración de descarga predeterminada.

## Comandos desde el monitor

Enviar cada palabra con Enter; se aceptan LF, CR o CRLF y mayúsculas/minúsculas. Los alias también requieren Enter.

| Comando | Acción |
| --- | --- |
| iniciar o i | Comenzar/continuar integrando; esperar el primer promedio y terminar la calibración si corresponde |
| pausar o p | Congelar Ah/Wh y tiempo integrado; seguir viendo volts, amperes y potencia |
| reiniciar o r | Borrar consumo y tiempo; quedar pausado con capacidad completa |
| estado | Mostrar el reporte inmediatamente |
| test | En pausa, ejecutar la hora virtual de aceptación; no toca tu sesión |
| calibrar | Solo en modo ACS real y en pausa: medir nuevamente el offset SIN CARGA |
| ayuda o ? | Mostrar los comandos |

El reporte sale cada segundo. Una vez disponibles las primeras mediciones, el valor leído no suspende la integración ni se sustituye por cero por estar fuera de una escala. Se conserva el último promedio hasta completar el siguiente, integrando con el tiempo real transcurrido. La pausa manual o una calibración solicitada no se suman retrospectivamente. El tiempo mostrado es el que se integró, no el total desde el encendido.

## Qué observar

Con fuentes estables, después del filtro se esperan aproximadamente estas relaciones:

| GPIO34 | Pack | GPIO35 | Corriente | Potencia |
| --- | --- | --- | --- | --- |
| 0,450 V | 48 V | 2,500 V | 0 A | 0 W |
| 0,450 V | 48 V | 2,840 V | 8,5 A | 408 W |
| 0,850 V | 50 V | 2,840 V | 8,5 A | 425 W |
| 1,250 V | 52 V | 2,700 V | 5 A | 260 W |
| 1,250 V | 52 V | 2,840 V | 8,5 A | 442 W |

En pausa, los valores instantáneos cambian al mover las fuentes, pero el consumo sigue en cero. Enviar iniciar: manteniendo 48 V / 8,5 A durante 60 s integrados, se consumen aproximadamente 0,1417 Ah y 6,8 Wh. Enviar pausar: esos acumulados deben quedar congelados incluso al variar ambas fuentes. Enviar reiniciar: quedan en cero y la sesión sigue pausada.

El ADC del ESP32 y las fuentes tienen error y ruido; una entrada medida físicamente como 2,840 V puede no producir exactamente 8,500 A. El reporte incluye el promedio de volts medidos en el pin para comparar con el multímetro. Si hace falta ajustar la escala, cambiar los parámetros centrales. El banco no calibra automáticamente el cero ni absorbe una corriente pequeña como reposo.

## Parámetros: un solo archivo

Editar src/Configuracion.h, recompilar y cargar. No son parámetros persistentes ni comandos de configuración en ejecución.

| Parámetros | Qué ajustan |
| --- | --- |
| PIN_TENSION, PIN_CORRIENTE | Pines ADC1; por defecto 34/35 |
| ENTRADA_TENSION_MIN_V, ENTRADA_TENSION_MAX_V | Dos volts medidos en GPIO34 que definen la escala |
| PACK_MIN_V, PACK_MAX_V | Dos tensiones de pack correspondientes a esos puntos |
| OFFSET_CORRIENTE_V | Cero nominal en la salida del ACS reconstruida |
| SENSIBILIDAD_CORRIENTE_V_POR_A | Variación de salida por ampere; nominal 0,040 V/A |
| FACTOR_SALIDA_SENSOR_SOBRE_ADC | V_salida_sensor / V_GPIO35; para fuentes directas queda en 1,0 |
| SIGNO_CORRIENTE | +1 o -1 para la orientación de descarga |
| CALIBRACION_CORRIENTE | Cero fijo o automático, en modos ACS reales |
| ESTABILIZACION_CORRIENTE_MS, MUESTRAS_CALIBRACION, INTERVALO_CALIBRACION_MS | Espera y muestreo del cero, sin bloquear loop |
| AJUSTAR_OFFSET_EN_REPOSO y parámetros asociados | Ajuste opcional en modo de calibración automática; por defecto apagado |
| CAPACIDAD_PACK_AH, TENSION_NOMINAL_PACK_V | Capacidad y energía nominal de referencia, por defecto 17 Ah y 816 Wh |
| INTERVALO_TENSION_MS, MUESTRAS_TENSION, INTERVALO_CORRIENTE_MS, MUESTRAS_CORRIENTE | Frecuencia de muestreo y tamaño del promedio simple |
| HABILITAR_SENSORES_AUXILIARES | Activar GPS, temperatura e ISR inductiva existentes; banco: false |

La interpolación de tensión es:
pack = PACK_MIN_V + (pinV - ENTRADA_TENSION_MIN_V) × (PACK_MAX_V - PACK_MIN_V) / (ENTRADA_TENSION_MAX_V - ENTRADA_TENSION_MIN_V).

La conversión de corriente es:
I = SIGNO_CORRIENTE × (pinV × FACTOR_SALIDA_SENSOR_SOBRE_ADC - offsetEfectivo) / SENSIBILIDAD_CORRIENTE_V_POR_A.

La tensión nominal de energía es independiente del máximo medible: cambiar PACK_MAX_V no cambia automáticamente los Wh nominales. Los valores de 17 Ah, 48 V y 8,5 A de la función test son el caso matemático de aceptación, no copias de la calibración de operación.

En banco, offsetEfectivo es OFFSET_CORRIENTE_V. Con ACS real y calibración automática, se espera la estabilización y se mide ese cero con corriente cero; se muestra el valor efectivo, pero no se modifica la constante del archivo. Para sensibilidad nominal de 40 mV/A se presupone el ACS alimentado a 5 V; verificarla si se usa otra alimentación.

Corriente usa un promedio simple de las 15 muestras tomadas cada 5 ms (aprox. 75 ms por resultado); tensión promedia las 5 muestras tomadas cada 50 ms (aprox. 250 ms). No se descartan extremos ni se redondea a pasos de tensión. Los puntos 0,450 y 1,250 V definen la conversión, no límites de aceptación: por ejemplo, 1,270 V se convierte en 52,1 V. El reporte a 1 s es independiente de la integración.

## Pasar a sensores reales

Cambiar MODO a Config::ModoEnergia::SENSORES_REALES. Antes de conectar, verificar el acondicionamiento real de ambos sensores para los límites eléctricos del ESP32 y configurar su escala/factor medidos. No se inventa ningún divisor resistivo.

Con CALIBRACION_CORRIENTE=AUTOMATICA_EN_CERO, encender sin corriente y mantener la carga desconectada hasta terminar la calibración (por defecto alrededor de 1,5 s más el primer grupo de medición). Si se calibra con corriente circulando, esa corriente se convierte en el nuevo cero. También se puede usar OFFSET_FIJO con un cero ya medido. Los modos con fuentes siempre fuerzan offset fijo y desactivan ajuste en reposo.

La escala de tensión 0,450–1,250 V a 48–52 V proviene del módulo existente: confirmar que corresponda al circuito real. Para una prueba parcial se conservan VOLTAJE_ADC_INYECTADO (fuente de corriente, pack fijo), ACS_TENSION_FIJA (ACS real, pack fijo) y REFERENCIA_SIMULADA (magnitudes físicas sin ADC), todos con sesión inicialmente pausada.

## Alcance del cálculo

Energia recibe volts, amperes y segundos; SesionEnergia controla pausa/reanudación usando la diferencia real de milisegundos. No hay ADC ni calibración en el algoritmo energético, ni cálculos energéticos en interrupciones.

SOC se basa en Ah: subir la fuente de tensión no recarga automáticamente la batería. Wh restantes = energía nominal configurada menos Wh consumidos. Los restantes y SOC se limitan a cero; los consumidos pueden superar el presupuesto nominal. Corriente/potencia negativa se muestran, pero no se contabiliza recuperación de energía. Cada reinicio del ESP32 borra la sesión. No se implementa persistencia, Peukert, temperatura de capacidad ni límites reales de descarga.

## Validación en computadora

Estas pruebas ejecutan el cálculo y los módulos de adquisición con un ADC sustituido por señales de prueba. No sustituyen una prueba eléctrica con ESP32. Desde Codigo_integrado, con g++ en Linux:

```sh
g++ -std=c++11 -Wall -Wextra -Werror -pedantic src/Sensores/Energia/Energia.cpp test/prueba_energia_host.cpp -o /tmp/prueba_energia
/tmp/prueba_energia
g++ -std=c++11 -Wall -Wextra -Werror -pedantic -Itest/arduino_simulado src/Sensores/Energia/Energia.cpp src/Sensores/Tension/Tension.cpp src/Sensores/Corriente/Corriente.cpp test/prueba_adquisicion_host.cpp -o /tmp/prueba_adc
/tmp/prueba_adc
```

Las pruebas de PC se ejecutan con estos comandos, no con pio test. El Build normal de PlatformIO compila solo src y no usa el Arduino simulado que está en test.

Validación de esta actualización: compilación PlatformIO esp32dev; caso de una hora = 408 W, 8,5 Ah consumidos/restantes, 408 Wh consumidos/restantes y 50 % SOC. La prueba ADC comprueba 60 s continuos = 6,8 Wh incluso con ADC crudo 4095; también comprueba promedio simple con todas las muestras, conversión sin recortes, fuentes variables y calibración. La prueba de energía cubre pausa, reinicio, tiempo variable, desbordamiento de millis() y límites de SOC. Los tests adjuntos usan los parámetros predeterminados. No se ejecutó en una placa física durante la preparación.
