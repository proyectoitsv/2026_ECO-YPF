#ifndef ARDUINO_DISPLAY_HOST_H
#define ARDUINO_DISPLAY_HOST_H

// Soporte mínimo para probar el código de producción en una computadora.
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include "freertos/FreeRTOS.h"

constexpr uint32_t SERIAL_8N1 = 0x800001c;
class HardwareSerial {
public:
    unsigned long baud = 0;
    uint32_t config = 0;
    int rx = -1, tx = -1;
    std::vector<uint8_t> bytes;
    std::string texto;
    void begin(unsigned long b, uint32_t c = SERIAL_8N1, int r = -1, int t = -1) {
        baud = b; config = c; rx = r; tx = t;
    }
    size_t write(uint8_t b) { bytes.push_back(b); return 1; }
    template<typename T> void print(T v) {
        std::ostringstream s; s << v; texto += s.str();
    }
    void print(double v, int decimales) {
        std::ostringstream s;
        s << std::fixed << std::setprecision(decimales) << v; texto += s.str();
    }
    template<typename T> void println(T v) { print(v); texto += '\n'; }
    void println(double v, int decimales) { print(v, decimales); texto += '\n'; }
    void println() { texto += '\n'; }
    int available() { return 0; }
    int read() { return -1; }
};
extern HardwareSerial Serial;
extern HardwareSerial Serial1;
uint32_t millis();

#endif
