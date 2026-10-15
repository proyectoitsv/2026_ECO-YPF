#ifndef DISPLAY_COM11441_H
#define DISPLAY_COM11441_H

#include <Arduino.h>

// Controlador UART TTL para los 4 dígitos del SparkFun COM-11441.
// Solo TareaDisplay utiliza este objeto y su UART.
class DisplayCOM11441 {
public:
    DisplayCOM11441(HardwareSerial& serial, int txPin, int rxPin = -1);
    void begin(uint32_t baud = 9600);
    void clear();
    void setBrightness(uint8_t value);
    void showText(const char* text);
    void showNumber(int value);
    void showBatteryPercentage(int percentage);

private:
    HardwareSerial& serial_;
    int txPin_;
    int rxPin_;
};

#endif
