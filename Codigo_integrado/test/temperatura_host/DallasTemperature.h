#pragma once
#include <assert.h>
#include <vector>
#include <utility>
#include "OneWire.h"
namespace FakeTemperatura {
extern bool parasita1;
extern std::vector<std::pair<unsigned,uint32_t>> solicitudes;
}
class DallasTemperature {
    uint8_t pin;
public:
    explicit DallasTemperature(OneWire* o) : pin(o->pin) {}
    void begin() {}
    bool isParasitePowerMode() { return pin==25 && FakeTemperatura::parasita1; }
    void setResolution(uint8_t n) { assert(n==12); }
    void setWaitForConversion(bool esperar) { assert(!esperar); }
    void requestTemperatures() { FakeTemperatura::solicitudes.push_back({pin,millis()}); }
    float getTempCByIndex(uint8_t i) { assert(i==0); return pin==25 ? 30 : millis()<6000 ? 40 : -127; }
};
