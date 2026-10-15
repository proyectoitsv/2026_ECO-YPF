# Cambios de la integración microSD

Comparación contra `Display-Sparkfun` (`e4ca276507141481e6e8dc57f8706e6747a8eb69`, 14/10/2026). Todo está en la rama local `feature/registro-microsd`.

## Archivos nuevos

- `src/Registro/ConfigRegistro.h`: períodos, pines, tamaños y frescura.
- `src/Registro/RelojCore.*`, `Reloj.*`: UTC/Unix, hora argentina, interpolación monotónica y eventos de sincronización.
- `src/Registro/Resumen.*`: medias temporales y cobertura independientes de la energía.
- `src/Registro/CSV.*`: snapshot de 584 bytes copiable por valor, formato acotado y esquema de 71 columnas.
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

Sin diferencias frente al commit base: `src/Tareas/TareaDisplay.cpp`, `src/Display/Display_COM11441.*`, `src/Sensores/Energia/Energia.*`, `src/Sensores/Energia/SesionEnergia.h`, `src/Sensores/Corriente/*` y `src/Sensores/Tension/*`. El display conserva clear 2 s/3 s. No se cambiaron las fórmulas ni constantes eléctricas.

El cambio amplía FreeRTOS y los datos compartidos; no es una modificación local de un único archivo. Build PlatformIO y pruebas se detallan en `docs/microsd/VERIFICACION.md`.
