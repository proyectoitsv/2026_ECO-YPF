#pragma once
#include "../display_host/Arduino.h"
constexpr int INPUT = 0, ADC_11db = 3;
inline void pinMode(int, int) {}
inline void analogReadResolution(int) {}
inline void analogSetPinAttenuation(int, int) {}
uint32_t analogReadMilliVolts(int);
uint16_t analogRead(int);
