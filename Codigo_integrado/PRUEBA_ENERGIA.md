# Prueba de energía en ESP32-WROOM-32

El proyecto PlatformIO está en esta carpeta. Compilar con el entorno `esp32dev` del `platformio.ini` existente. El monitor serie usa 115200 baudios. Cambiar `upload_port = COM7` por el puerto real de la computadora si hace falta.

Si se recibió esta versión como ZIP mientras se habilita la escritura en GitHub: crear una rama desde `main`, copiar la carpeta `Codigo_integrado` del ZIP sobre la carpeta homónima del clon y revisar el cambio antes de subirlo. Así `main` queda intacta. En la carpeta del proyecto se puede ejecutar `pio run`, después `pio run -t upload` con la placa conectada y finalmente `pio device monitor -b 115200`.

En `src/main.cpp`, seleccionar `MODO_ENERGIA`:

| Modo | Fuente de corriente | Fuente de tensión del pack | Para qué sirve |
| --- | --- | --- | --- |
| `REFERENCIA_SIMULADA` (predeterminado) | Salida ACS simulada 2,840 V, offset 2,500 V → 8,5 A | 48 V fijos | Probar el programa sin conectar ADC ni baterías |
| `VOLTAJE_ADC_INYECTADO` | 2,840 V aplicados al GPIO35, offset nominal fijo 2,500 V | 48 V fijos | Probar el ADC con una señal segura y estable |
| `ACS_TENSION_FIJA` | ACS758 real en GPIO35; offset medido en arranque sin corriente | 48 V fijos | Probar corriente real antes del módulo de tensión |
| `SENSORES_REALES` | ACS758 real en GPIO35; offset medido en arranque sin corriente | `MedidaTension.voltajeBateria` del módulo Tension en GPIO34 | Probar tensión y corriente reales |

## Prueba acelerada

En cada arranque se ejecuta en memoria una hora virtual de 3600 intervalos de un segundo. El monitor debe mostrar `PRUEBA PATRON: OK`, 408 W, 8,5 Ah consumidos, 8,5 Ah restantes, 408 Wh consumidos, 408 Wh restantes y 50 % de batería. Esta prueba usa un objeto energético separado: los acumulados de operación real comienzan en cero después de la inicialización. En el modo predeterminado, después se muestra la integración según el tiempo real de `millis()`, sin esperar ni simular una hora en el `loop()`.

También se puede ejecutar la prueba matemática en una computadora con `g++`:

```sh
g++ -std=c++11 -Wall -Wextra -pedantic src/Sensores/Energia/Energia.cpp test/prueba_energia_host.cpp -o /tmp/prueba_energia
/tmp/prueba_energia
```

## Conexiones y calibración

- GPIO34 está reservado al módulo de tensión existente; GPIO35, entrada ADC1, se usa para corriente. GPS usa GPIO16/17, temperatura GPIO25/33 e inductivo GPIO26. Todas las masas de las señales analógicas deben tener la referencia que exige el circuito de acondicionamiento.
- **No conectar directamente al GPIO35 una salida del ACS758 de 5 V que pueda superar el límite admitido por el ESP32.** Verificar y medir el circuito real de adaptación antes de conectar el sensor o el pack. Este código deja `FACTOR_SALIDA_SENSOR_SOBRE_ADC = 1.0f` en `Corriente.cpp` como marcador sin divisor; si existe divisor/adaptador, configurar el factor medido `V_salida_sensor / V_entrada_GPIO35`. No se presupone ninguna resistencia ni circuito.
- `VOLTAJE_ADC_INYECTADO` presupone 2,840 V **en el pin** solo cuando el factor es 1,0. Si se añade un divisor, ajustar el voltaje de entrada para que la salida reconstruida del ACS sea 2,840 V. El ADC tiene tolerancias; este modo comprueba la ruta física, y la prueba virtual comprueba las cifras exactas.
- En los modos ACS reales, arrancar con corriente **cero** durante la calibración del offset (500 muestras, alrededor de un segundo). Con 8,5 A circulando, el arranque convertiría aproximadamente 2,840 V en el nuevo cero y mostraría cerca de 0 A. El ajuste lento de offset en reposo se conserva de la rama del sensor.
- La sensibilidad nominal usada es 0,040 V/A. El signo positivo presupone que el ACS está orientado para que la descarga eleve la salida por encima del offset. Si la polaridad de montaje es inversa, corregir la orientación o la convención de signo después de verificarla; la energía consumida solo integra corriente positiva.
- En el modo real, `Tension.cpp` ya asigna 0,450–1,250 V **en GPIO34** a 48–52 V de pack. Confirmar con instrumentos la escala y el circuito de entrada antes de tomar los Wh como reales. Su `porcentaje` basado en tensión es independiente del SOC basado en Ah de `GestorEnergia`.

La batería son cuatro unidades de 12 V / 17 Ah **en serie**: el pack se configura como 48 V nominales, **17 Ah** y 816 Wh ideales. `Wh restantes` se calcula como 816 Wh menos los Wh consumidos; el SOC se estima desde Ah restantes. Ambos son ideales, se limitan a cero al agotarse y se reinician cuando se reinicia el ESP32. No hay persistencia, Peukert, límites reales de descarga ni recuperación de energía en esta etapa.
