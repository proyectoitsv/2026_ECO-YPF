// =============================================================
//  MEDIDOR DE TEMPERATURA - DOS SENSORES DS18B20 INDEPENDIENTES
//  Sin librerías externas - Protocolo 1-Wire manual
//  Sensor 1 (batería A): GPIO 18
//  Sensor 2 (batería B): GPIO 19
//  Reporte serial: cada 2 segundos
//  Compatible con: Medidor RPM/Frecuencia (rpm_setup / rpm_loop)
// =============================================================

// -------------------------------------------------------------
//  SECCIÓN 1: DEFINES DE CONFIGURACIÓN
// -------------------------------------------------------------
#define PIN_SENSOR_1          18
#define PIN_SENSOR_2          19
#define INTERVALO_TEMP_MS     2000
#define SERIAL_BAUDRATE       9600   // Debe coincidir con el código RPM

// Comandos del protocolo 1-Wire / DS18B20
#define DS18B20_SKIP_ROM      0xCC
#define DS18B20_CONVERT_T     0x44
#define DS18B20_READ_SCRATCH  0xBE

// Valor centinela para indicar lectura fallida
#define TEMP_ERROR            -999.0f

// -------------------------------------------------------------
//  SECCIÓN 2: VARIABLES GLOBALES (no hay ISR en este módulo)
// -------------------------------------------------------------
// (Sin variables volátiles - el DS18B20 se lee de forma síncrona)

// -------------------------------------------------------------
//  SECCIÓN 3: VARIABLES DE CÁLCULO (usadas solo en temp_loop)
// -------------------------------------------------------------
uint32_t tiempoUltimaLecturaTemp_ms = 0;

float tempSensor1_C = 0.0f;
float tempSensor2_C = 0.0f;

// -------------------------------------------------------------
//  SECCIÓN 4: FUNCIONES 1-WIRE MANUALES (núcleo del protocolo)
// -------------------------------------------------------------

// ── Genera el pulso de reset e identifica presencia del sensor ──
// Devuelve true si el sensor respondió, false si no hay dispositivo.
bool oneWire_Reset(uint8_t pin) {
  // Tirar la línea a LOW por 480 µs mínimo (reset pulse)
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delayMicroseconds(480);

  // Soltar la línea y esperar la ventana de presencia (15-60 µs)
  pinMode(pin, INPUT_PULLUP);
  delayMicroseconds(70);

  // Leer el pulso de presencia: el sensor la pone en LOW
  bool presencia = (digitalRead(pin) == LOW);
  delayMicroseconds(410); // Completar el slot de 480 µs

  return presencia;
}

// ── Escribe UN bit en el bus 1-Wire ──
void oneWire_WriteBit(uint8_t pin, uint8_t bit) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  if (bit) {
    delayMicroseconds(6);    // Slot '1': pulso corto, luego soltar
    pinMode(pin, INPUT_PULLUP);
    delayMicroseconds(64);
  } else {
    delayMicroseconds(60);   // Slot '0': mantener LOW todo el slot
    pinMode(pin, INPUT_PULLUP);
    delayMicroseconds(10);
  }
}

// ── Lee UN bit del bus 1-Wire ──
uint8_t oneWire_ReadBit(uint8_t pin) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delayMicroseconds(6);       // Pulso de iniciación de lectura (≥1 µs)
  pinMode(pin, INPUT_PULLUP);
  delayMicroseconds(9);       // Esperar centro del slot de muestreo
  uint8_t bit = digitalRead(pin);
  delayMicroseconds(55);      // Completar el slot de 70 µs
  return bit;
}

// ── Escribe un byte completo (LSB primero, según estándar 1-Wire) ──
void oneWire_WriteByte(uint8_t pin, uint8_t byteVal) {
  for (uint8_t i = 0; i < 8; i++) {
    oneWire_WriteBit(pin, (byteVal >> i) & 0x01);
  }
}

// ── Lee un byte completo (LSB primero) ──
uint8_t oneWire_ReadByte(uint8_t pin) {
  uint8_t resultado = 0;
  for (uint8_t i = 0; i < 8; i++) {
    if (oneWire_ReadBit(pin)) {
      resultado |= (1 << i);
    }
  }
  return resultado;
}

// ── Secuencia completa de lectura de temperatura del DS18B20 ──
// Retorna la temperatura en °C, o TEMP_ERROR si falla la comunicación.
float ds18b20_LeerTemperatura(uint8_t pin) {

  // PASO 1: Iniciar conversión de temperatura
  if (!oneWire_Reset(pin)) return TEMP_ERROR;
  oneWire_WriteByte(pin, DS18B20_SKIP_ROM);    // No importa la dirección ROM
  oneWire_WriteByte(pin, DS18B20_CONVERT_T);   // Ordenar conversión

  // PASO 2: Esperar conversión (máximo 750 ms para resolución 12-bit)
  // En lugar de un delay fijo, esperamos la señal de "listo" del sensor
  uint32_t inicio = millis();
  while (oneWire_ReadBit(pin) == 0) {
    if ((millis() - inicio) > 1000) return TEMP_ERROR; // Timeout de seguridad
    delay(10);
  }

  // PASO 3: Leer el Scratchpad (registro interno con los 9 bytes de datos)
  if (!oneWire_Reset(pin)) return TEMP_ERROR;
  oneWire_WriteByte(pin, DS18B20_SKIP_ROM);
  oneWire_WriteByte(pin, DS18B20_READ_SCRATCH);

  // Solo necesitamos los 2 primeros bytes: LSB y MSB del dato de temperatura
  uint8_t tempLSB = oneWire_ReadByte(pin);
  uint8_t tempMSB = oneWire_ReadByte(pin);

  // PASO 4: Convertir los 2 bytes a temperatura en °C
  // El DS18B20 devuelve el valor en complemento a 2, con resolución de 1/16 °C
  int16_t rawTemp = (int16_t)((tempMSB << 8) | tempLSB);
  float temperatura_C = (float)rawTemp / 16.0f;

  return temperatura_C;
}

// -------------------------------------------------------------
//  SECCIÓN 5: FUNCIÓN DE INICIALIZACIÓN
// -------------------------------------------------------------
void temp_setup() {
  // Serial.begin() ya fue llamado por rpm_setup() con el mismo baudrate.
  // Si usás este código de forma INDEPENDIENTE (sin el código RPM),
  // descomentá la siguiente línea:
  // Serial.begin(SERIAL_BAUDRATE);
  // delay(500);

  Serial.println("============================================");
  Serial.println("  Medidor de Temperatura - DS18B20 x2");
  Serial.println("  Sin librerias externas - 1-Wire manual");
  Serial.println("============================================");
  Serial.print("  Sensor 1 (Bateria A)  : GPIO ");
  Serial.println(PIN_SENSOR_1);
  Serial.print("  Sensor 2 (Bateria B)  : GPIO ");
  Serial.println(PIN_SENSOR_2);
  Serial.print("  Intervalo de lectura  : ");
  Serial.print(INTERVALO_TEMP_MS);
  Serial.println(" ms");
  Serial.println("============================================");
  Serial.println();

  // Los pines se configuran como INPUT_PULLUP en reposo (línea 1-Wire en HIGH)
  pinMode(PIN_SENSOR_1, INPUT_PULLUP);
  pinMode(PIN_SENSOR_2, INPUT_PULLUP);

  tiempoUltimaLecturaTemp_ms = millis();
  Serial.println("Sensores listos. Iniciando lecturas...");
  Serial.println();
  Serial.println("  Sensor 1 [Bat. A]  |  Sensor 2 [Bat. B]");
  Serial.println("  ------------------|-------------------");
}

// -------------------------------------------------------------
//  SECCIÓN 6: FUNCIÓN DE LOOP PRINCIPAL
// -------------------------------------------------------------
void temp_loop() {
  uint32_t ahora_ms = millis();

  if ((ahora_ms - tiempoUltimaLecturaTemp_ms) >= INTERVALO_TEMP_MS) {
    tiempoUltimaLecturaTemp_ms = ahora_ms;

    // ── Leer ambos sensores secuencialmente ──
    // (No se pueden leer en paralelo porque cada bus 1-Wire es independiente
    //  y el protocolo es síncrono: bloquea ~750 ms por lectura)
    tempSensor1_C = ds18b20_LeerTemperatura(PIN_SENSOR_1);
    tempSensor2_C = ds18b20_LeerTemperatura(PIN_SENSOR_2);

    // ── Imprimir resultados ──
    Serial.print("  ");
    if (tempSensor1_C == TEMP_ERROR) {
      Serial.print("  ERROR (sin resp.)");
    } else {
      Serial.print(tempSensor1_C, 2);
      Serial.print(" C            ");
    }

    Serial.print(" |  ");

    if (tempSensor2_C == TEMP_ERROR) {
      Serial.println("  ERROR (sin resp.)");
    } else {
      Serial.print(tempSensor2_C, 2);
      Serial.println(" C");
    }
  }
}

// -------------------------------------------------------------
//  SECCIÓN 7: SETUP Y LOOP DE ARDUINO
// -------------------------------------------------------------
void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  delay(500);
  temp_setup();
}

void loop() {
  temp_loop();
}
