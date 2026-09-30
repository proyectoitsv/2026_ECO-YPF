#ifndef ARDUINO_SIMULADO_PARA_TEST_H
#define ARDUINO_SIMULADO_PARA_TEST_H
#include <stdint.h>
// Sustituto mínimo exclusivo de las pruebas en PC; no se copia a src/.
const int INPUT = 0;
enum adc_attenuation_t { ADC_11db };
uint32_t millis();
uint32_t analogReadMilliVolts(uint8_t pin);
uint16_t analogRead(uint8_t pin);
void pinMode(uint8_t pin, int modo);
void analogReadResolution(uint8_t bits);
void analogSetPinAttenuation(uint8_t pin, adc_attenuation_t atenuacion);
template<class T> T constrain(T x, T minimo, T maximo) {
    return x < minimo ? minimo : x > maximo ? maximo : x;
}
#endif
