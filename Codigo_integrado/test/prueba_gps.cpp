// Parser TinyGPS++ real y wrapper de producción, UART y reloj simulados.
#include <assert.h>
#include <string>
#include <iostream>
#include "../src/Sensores/GPS/GPS.h"
#include "../src/Registro/Reloj.h"
HardwareSerial Serial2;
static uint64_t ahora=1000000;
static ReferenciaUTC referencia;
static unsigned sincronizaciones=0;
uint32_t millis() { return ahora/1000; }
int64_t esp_timer_get_time() { return ahora; }
void sincronizarUTC(const FechaUTC& f,uint64_t us) {
    int64_t ms; assert(convertirUnix(f,ms));
    aplicarSincronizacion(referencia,ms,us); ++sincronizaciones;
}
HoraRegistro leerHora(uint64_t us) { return estimarHora(referencia,us,3000); }
void enviar(const std::string& cuerpo) {
    uint8_t crc=0; for (char c:cuerpo) crc^=uint8_t(c);
    char sufijo[8]; snprintf(sufijo,sizeof(sufijo),"*%02X\r\n",crc);
    Serial2.entrada+="$"+cuerpo+sufijo;
    actualizarGPS();
    assert(!Serial2.available());
}
int main() {
    inicializarGPS();
    assert(Serial2.baud==9600 && Serial2.rx==16 && Serial2.tx==17 && ahora==1000000);
    assert(leerHora(ahora).fuente==FuenteHora::SIN_HORA);
    // RMC sin fix: fecha/hora sí son utilizables, latitud/velocidad aún no.
    enviar("GPRMC,235959.00,V,,,,,,,311226,,,N");
    assert(sincronizaciones==1 && !obtenerDatosGPS().ubicacionValida);
    const int64_t antes=leerHora(ahora).unixMs;
    ahora+=1000000;
    enviar("GPGGA,000000.00,,,,,0,00,99.99,,,,,,");
    assert(sincronizaciones==1 && leerHora(ahora).unixMs==antes+1000);
    // Fecha vacía no debe reciclar el 31/12 en un RMC después de medianoche.
    enviar("GPRMC,000000.00,V,,,,,,,,,,N");
    assert(sincronizaciones==1);
    enviar("GNRMC,000000.00,A,3400.0000,S,05800.0000,W,0.0,123.4,010127,,,A");
    assert(sincronizaciones==2 && leerHora(ahora).unixMs==antes+1000);
    DatosGPS d=obtenerDatosGPS();
    assert(d.ubicacionValida && d.velocidadValida && d.velocidadKmH==0 && !d.rumboValido);
    assert(d.anio==2026 && d.mes==12 && d.dia==31 && d.hora==21); // UTC-3 una vez.
    const uint64_t posicion=d.posicionUs;
    ahora+=4000000;
    enviar("GPGGA,000004.00,,,,,0,00,99.99,,,,,,");
    d=obtenerDatosGPS();
    assert(d.posicionUs==posicion && d.satelitesValidos && d.satelites==0);
    assert(leerHora(ahora).fuente==FuenteHora::ESTIMADA && leerHora(ahora).edadMs==4000);
    // Recuperación con ajuste de UTC: ningún reloj relativo depende del ajuste.
    enviar("GPRMC,000003.00,A,3400.0000,S,05800.0000,W,10.0,123.4,010127,,,A");
    d=obtenerDatosGPS();
    assert(sincronizaciones==3 && d.posicionUs==ahora && d.rumboValido && d.velocidadKmH>18);
    assert(leerHora(ahora).fuente==FuenteHora::GPS && leerHora(ahora).unixMs==antes+4000);
    assert(Serial2.salida.size()==10); // ECO enviado después de 2 s, sin delay bloqueante.
    std::cout<<"OK: NMEA real, hora sin fix, medianoche GP/GN, RMC vacio, edades por magnitud, perdida/recuperacion y UART sin espera.\n";
}
