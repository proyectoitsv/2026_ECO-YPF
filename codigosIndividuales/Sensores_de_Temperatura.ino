// =============================================================
// MEDIDOR DE TEMPERATURA - DOS SENSORES DS18B20 INDEPENDIENTES
//
// Librerías:
//   - OneWire
//   - DallasTemperature
//
// Sensor 1 (Batería A): GPIO 18
// Sensor 2 (Batería B): GPIO 19
//
// Reporte por Serial: cada 2 segundos
// Compatible con sistema RPM (rpm_setup / rpm_loop)
// =============================================================


// -------------------------------------------------------------
// SECCIÓN 1: LIBRERÍAS
// -------------------------------------------------------------
#include <OneWire.h>
#include <DallasTemperature.h>


// -------------------------------------------------------------
// SECCIÓN 2: DEFINES DE CONFIGURACIÓN
// -------------------------------------------------------------
#define PIN_TEMP_SENSOR_1      18
#define PIN_TEMP_SENSOR_2      19

#define INTERVALO_TEMP_MS      2000
#define SERIAL_BAUDRATE_TEMP   9600


// -------------------------------------------------------------
// SECCIÓN 3: OBJETOS DE LIBRERÍA
//
// Cada sensor posee su propio bus OneWire independiente.
// -------------------------------------------------------------
OneWire oneWireBus1(PIN_TEMP_SENSOR_1);
OneWire oneWireBus2(PIN_TEMP_SENSOR_2);

DallasTemperature sensorBateriaA(&oneWireBus1);
DallasTemperature sensorBateriaB(&oneWireBus2);


// -------------------------------------------------------------
// SECCIÓN 4: VARIABLES GLOBALES
// -------------------------------------------------------------
uint32_t tiempoUltimaLecturaTemp_ms = 0;

float tempBateriaA_C = 0.0f;
float tempBateriaB_C = 0.0f;


// -------------------------------------------------------------
// SECCIÓN 5: FUNCIÓN DE LECTURA DE SENSOR
//
// Realiza:
//   requestTemperatures()
//   getTempCByIndex(0)
//
// Devuelve:
//   Temperatura en °C
//   -999.0f si el sensor no responde
// -------------------------------------------------------------
float temp_LeerSensor(DallasTemperature &sensor)
{
    sensor.requestTemperatures();

    float lectura = sensor.getTempCByIndex(0);

    if (lectura == DEVICE_DISCONNECTED_C)
    {
        return -999.0f;
    }

    return lectura;
}


// -------------------------------------------------------------
// SECCIÓN 6: INICIALIZACIÓN
// -------------------------------------------------------------
void temp_setup()
{
    // Si se integra con el código RPM,
    // comentar estas líneas:

    Serial.begin(SERIAL_BAUDRATE_TEMP);
    delay(500);

    Serial.println("============================================");
    Serial.println(" Medidor de Temperatura - DS18B20 x2");
    Serial.println(" Librerias: OneWire + DallasTemperature");
    Serial.println("============================================");

    Serial.print(" Sensor 1 (Bateria A) : GPIO ");
    Serial.println(PIN_TEMP_SENSOR_1);

    Serial.print(" Sensor 2 (Bateria B) : GPIO ");
    Serial.println(PIN_TEMP_SENSOR_2);

    Serial.print(" Intervalo de lectura : ");
    Serial.print(INTERVALO_TEMP_MS);
    Serial.println(" ms");

    Serial.println("============================================");
    Serial.println();

    sensorBateriaA.begin();
    sensorBateriaB.begin();

    tiempoUltimaLecturaTemp_ms = millis();

    Serial.println("Sensores listos. Iniciando lecturas...");
    Serial.println();

    Serial.println(" Bateria A [°C] | Bateria B [°C]");
    Serial.println(" ---------------|----------------");
}


// -------------------------------------------------------------
// SECCIÓN 7: LOOP PRINCIPAL
// -------------------------------------------------------------
void temp_loop()
{
    uint32_t ahora_ms = millis();

    if ((ahora_ms - tiempoUltimaLecturaTemp_ms) >= INTERVALO_TEMP_MS)
    {
        tiempoUltimaLecturaTemp_ms = ahora_ms;

        // Leer sensores
        tempBateriaA_C = temp_LeerSensor(sensorBateriaA);
        tempBateriaB_C = temp_LeerSensor(sensorBateriaB);

        // Mostrar resultados
        Serial.print(" ");

        if (tempBateriaA_C == -999.0f)
        {
            Serial.print("ERROR (sin resp.)");
        }
        else
        {
            Serial.print(tempBateriaA_C, 2);
            Serial.print(" C");
        }

        Serial.print(" | ");

        if (tempBateriaB_C == -999.0f)
        {
            Serial.println("ERROR (sin resp.)");
        }
        else
        {
            Serial.print(tempBateriaB_C, 2);
            Serial.println(" C");
        }
    }
}


// -------------------------------------------------------------
// SECCIÓN 8: SETUP Y LOOP DE ARDUINO
// -------------------------------------------------------------
void setup()
{
    temp_setup();
}

void loop()
{
    temp_loop();
}