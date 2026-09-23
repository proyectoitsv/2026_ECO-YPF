#include "GPS.h"
#include <TinyGPSPlus.h>

// --- CONFIGURACIÓN SERIAL DEL GPS ---
#define RXD2 16
#define TXD2 17
#define GPS_BAUD 9600

// Argentina = UTC-3, sin horario de verano
#define OFFSET_HORAS_ARG -3

static TinyGPSPlus gps;
static DatosGPS datosActuales = {};
static bool nuevaLectura = false;

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

static uint8_t diasDelMes(uint8_t mes, uint16_t anio) {
    const uint8_t dias[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (mes == 2 && ((anio % 4 == 0 && anio % 100 != 0) || anio % 400 == 0)) {
        return 29;
    }
    return dias[mes - 1];
}

static void ajustarHoraArgentina(uint8_t &hora, uint8_t &dia, uint8_t &mes, uint16_t &anio) {
    int horaLocal = (int)hora + OFFSET_HORAS_ARG;

    if (horaLocal < 0) {
        horaLocal += 24;
        if (dia == 1) {
            mes--;
            if (mes == 0) {
                mes = 12;
                anio--;
            }
            dia = diasDelMes(mes, anio);
        } else {
            dia--;
        }
    }

    hora = (uint8_t)horaLocal;
}

void inicializarGPS() {
    Serial2.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
    Serial.println("GPS Serial (Serial2) iniciado a 9600 baudios.");
    delay(2000);
    activarModoAhorro();
}

void actualizarGPS() {
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

        if (velocidad > 0.0 && gps.course.isValid()) {
            datosActuales.rumboGrados = gps.course.deg();
            datosActuales.rumboValido = true;
        } else {
            datosActuales.rumboGrados = 0.0;
            datosActuales.rumboValido = false;
        }

        datosActuales.hdop = gps.hdop.value() / 100.0;
        datosActuales.satelites = gps.satellites.value();

        if (gps.date.isValid() && gps.time.isValid()) {
            uint16_t anio = gps.date.year();
            uint8_t  mes  = gps.date.month();
            uint8_t  dia  = gps.date.day();
            uint8_t  hora = gps.time.hour();

            ajustarHoraArgentina(hora, dia, mes, anio);

            datosActuales.anio    = anio;
            datosActuales.mes     = mes;
            datosActuales.dia     = dia;
            datosActuales.hora    = hora;
            datosActuales.minuto  = gps.time.minute();
            datosActuales.segundo = gps.time.second();
        }

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