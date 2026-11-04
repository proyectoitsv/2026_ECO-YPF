#pragma once
#include <stdint.h>
struct OneWire { uint8_t pin; explicit OneWire(uint8_t p) : pin(p) {} };
