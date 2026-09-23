#include "GPS.h"
#include <TinyGPSPlus.h>

// --- CONFIGURACIÓN SERIAL DEL GPS ---
#define RXD2 16
#define TXD2 17
#define GPS_BAUD 9600

static TinyGPSPlus gps;
static DatosGPS datosActuales = {};
static bool nuevaLectura = false;

// Comando UBX para activar el Power Save Mode (PSM) en el NEO-6M
const byte UBX_ECO_MODE[] PROGMEM = {
    0xB5, 0x62, 0x06, 0x11, 0x02, 0x00, 0x08, 0x01, 0x22, 0x92
};

static void activarModoAhorro() {
    Serial.println("Enviando comando de ahorro de energía al NEO-6M...");
    for (size_t i = 0; i < sizeof(UBX_ECO_MODE); i++) {
        Serial2.write(pgm_read_byte(&UBX_ECO_MODE[i]));
    }
    Serial.println("Comando enviado.");
}

void inicializarGPS() {
    Serial2.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
    Serial.println("GPS Serial (Serial2) iniciado a 9600 baudios.");

    // Esperar a que el GPS encienda bien antes de mandarle el comando
    delay(2000);
    activarModoAhorro();
}

void actualizarGPS() {
    // Lee todo lo disponible sin bloquear (a diferencia del .ino original)
    while (Serial2.available() > 0) {
        gps.encode(Serial2.read());
    }

    if (gps.location.isUpdated()) {
        datosActuales.ubicacionValida = gps.location.isValid();
        datosActuales.latitud = gps.location.lat();
        datosActuales.longitud = gps.location.lng();

        float velocidad = 0.0;
        if (gps.speed.isValid()) {
            velocidad = gps.speed.kmph();
        }
        if (velocidad < 2.0) {
            velocidad = 0.0;
        }
        datosActuales.velocidadKmH = velocidad;

        datosActuales.rumboGrados = gps.course.isValid() ? gps.course.deg() : 0.0;
        datosActuales.hdop = gps.hdop.value() / 100.0;
        datosActuales.satelites = gps.satellites.value();

        datosActuales.anio   = gps.date.year();
        datosActuales.mes    = gps.date.month();
        datosActuales.dia    = gps.date.day();
        datosActuales.hora   = gps.time.hour();
        datosActuales.minuto = gps.time.minute();
        datosActuales.segundo = gps.time.second();

        nuevaLectura = true;
    }
}

bool datosGPSActualizados() {
    if (nuevaLectura) {
        nuevaLectura = false;
        return true;
    }
    return false;
}

DatosGPS obtenerDatosGPS() {
    return datosActuales;
}