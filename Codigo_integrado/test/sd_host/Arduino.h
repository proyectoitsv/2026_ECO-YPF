#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <sstream>
#include "freertos/FreeRTOS.h"
class HardwareSerial {
public:
    std::string salida;
    template<typename T> void print(T v) { std::ostringstream s; s<<v; salida+=s.str(); }
    template<typename T> void println(T v) { print(v); salida+='\n'; }
    int availableForWrite() { return 4096; }
    size_t write(const uint8_t* p,size_t n) { salida.append(reinterpret_cast<const char*>(p),n); return n; }
};
extern HardwareSerial Serial;
struct ESPFake { uint64_t getEfuseMac() { return 0xAABBCCDDEEFF; } uint32_t getFreeHeap() { return 196608; } };
extern ESPFake ESP;
uint32_t millis();

constexpr int OUTPUT=1,HIGH=1;
inline void pinMode(int,int) {}
inline void digitalWrite(int,int) {}
