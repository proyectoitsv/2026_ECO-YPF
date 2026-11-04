#pragma once
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <string>
#include <vector>
#include "../sd_host/freertos/FreeRTOS.h"
using byte=uint8_t;
#define PROGMEM
#define pgm_read_byte(p) (*(p))
constexpr uint32_t SERIAL_8N1=0x800001c;
constexpr double TWO_PI=6.28318530717958647692;
inline double radians(double d) { return d*TWO_PI/360; }
inline double degrees(double r) { return r*360/TWO_PI; }
inline double sq(double v) { return v*v; }
class HardwareSerial {
public:
    std::string entrada; size_t cursor=0;
    std::vector<uint8_t> salida;
    unsigned baud=0; int rx=-1,tx=-1;
    void begin(unsigned b,uint32_t,int r,int t) { baud=b; rx=r; tx=t; }
    int available() { return entrada.size()-cursor; }
    int read() { return available() ? uint8_t(entrada[cursor++]) : -1; }
    size_t write(uint8_t b) { salida.push_back(b); return 1; }
};
extern HardwareSerial Serial2;
uint32_t millis();
