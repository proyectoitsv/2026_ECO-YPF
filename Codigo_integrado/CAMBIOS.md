# Cambios del proyecto ECO YPF

## Actualización desde `SD-y-Timestamp` (`4934d7ab86693cba92a82bc5bc1248151a12554b`)

- CSV de 70 columnas, `schema_version=2`: se quita únicamente `monitor_omitidos` de cada fila; permanece en `sd estado` y en el cierre de metadatos. Se preservan las columnas ADC, los datos derivados, sus órdenes, fórmulas y precisiones.
- `MonitorCSV` muestra un panel con unidades, estados de datos ausentes o antiguos y el último error registrado; no se imprime la trama CSV en Serial. Usa la misma captura, un buffer fijo y un envío acotado.
- Una referencia UTC se copia antes de capturar datos o fijar el instante de nombre de archivo. Se evita asignar `SIN_HORA` por una actualización posterior del GPS.
- Los metadatos se reintentan independientemente del CSV y dejan una marca visible de recuperación; `sd estado` informa fallos, tareas disponibles y si falta ese archivo.
- Un fallo de creación de la escritora o de la captura deja el registrador detenido y reconocido como no disponible. Si falla solo el monitor, el CSV continúa guardándose.
- Glosario de las 70 columnas, comentarios en español y pruebas/ejemplos actualizados. Se explican el inicio nominal del SOC, la calibración efectiva y la diferencia entre verificar pines y habilitar tareas.

Los cálculos de Ah/Wh/SOC, el signo de corriente, la calibración numérica, pines, intervalos, cola, stacks, prioridades y ciclo del display conservan sus valores. No hubo commit ni publicación remota.

## Referencia histórica: integración inicial de microSD

Comparación contra `Display-Sparkfun` (`e4ca276507141481e6e8dc57f8706e6747a8eb69`, 14/10/2026). Todo está en la rama local `feature/registro-microsd`.

## Archivos nuevos

- `src/Registro/ConfigRegistro.h`: períodos, pines, tamaños y frescura.
- `src/Registro/RelojCore.*`, `Reloj.*`: UTC/Unix, hora argentina, interpolación monotónica y eventos de sincronización.
- `src/Registro/Resumen.*`: medias temporales y cobertura independientes de la energía.
- `src/Registro/CSV.*`: snapshot de 584 bytes copiable por valor, formato acotado y esquema original de 71 columnas (ahora 70).
- `src/Registro/Archivo.*`: creación única, header, sincronización, rename y protección de partes inciertas.
- `src/Registro/Registro.*`: tareas FreeRTOS de captura, SD/VSPI, control serial y monitor CSV.
- `docs/microsd/*`: diccionario exacto, verificación y fixtures sintéticos.
- `test/*`: pruebas host de núcleo, tareas SD, parser GPS, temperatura, regresión display y fakes de hardware/FreeRTOS.

## Archivos existentes modificados

| Archivo | Cambio |
| --- | --- |
| `platformio.ini` | Plataforma espressif32 6.9.0 y dependencias Arduino 2.x/OneWire/Dallas/TinyGPS fijadas. Puerto de referencia COM7 conservado. |
| `src/Configuracion.h` | Se habilitan los auxiliares; modo, calibraciones, escalas, conversiones y pines previos permanecen iguales. |
| `src/main.cpp` | Inicializa snapshots y logger; fija t0 antes de las tareas; crea tres tareas nuevas. Error de recursos SD se informa sin reiniciar el firmware. |
| `src/Rtos/Rtos.h` | Tarea de temperatura habilitada, heartbeats desactivados y diagnósticos con prefijo. |
| `src/Rtos/Compartidos.h/.cpp` | Marcas de adquisición, validez individuales, información coherente de energía/sensores y snapshot conjunto protegido. |
| `src/Sensores/GPS/GPS.h/.cpp` | Drenaje no bloqueante, UTC validado desde RMC completo no vacío y estado/edad por magnitud; zona local centralizada. |
| `src/Sensores/Temperatura/Temperatura.h/.cpp` | Resolución 12 bits, conversión asíncrona y detección de alimentación parasitaria. |
| `src/Tareas/TareaTemperatura.cpp` | Medición de ambos sensores cada 5 s también en movimiento; conversiones simultáneas y validez individual. |
| `src/Tareas/TareaGPS.cpp` | Reportes GPS extensos bajo flag y arranque UART no bloqueante. |
| `src/Tareas/TareaPCNT.cpp` | Timestamp monotónico IRAM de cada muestra y contadores publicados; 8 pulsos/vuelta conservados. |
| `src/Tareas/TareaEnergia.cpp` | Publica datos usados por Energía y atiende comandos SD desde el único lector Serial RX; calcula segmento al reiniciar. Algoritmo de acumulación intacto. |
| `test/prueba_display_host.cpp` | Stubs de reloj/registro para ejecutar regresión con la API compartida actualizada. |

## Verificación de partes preservadas

En aquella integración inicial, estos archivos de cálculo y display no tenían diferencias frente al commit de 14/10/2026. En la actualización actual se añadieron comentarios de aprendizaje, sin cambiar fórmulas ni constantes eléctricas. El display conserva clear 2 s/3 s.

El cambio amplía FreeRTOS y los datos compartidos; no es una modificación local de un único archivo. Build PlatformIO y pruebas se detallan en `docs/microsd/VERIFICACION.md`.
