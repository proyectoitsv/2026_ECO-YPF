#include "Display_COM11441.h"
#include <stdio.h>

DisplayCOM11441::DisplayCOM11441(HardwareSerial& serial, int txPin, int rxPin)
    : serial_(serial), txPin_(txPin), rxPin_(rxPin) {}

void DisplayCOM11441::begin(uint32_t baud) {
    // Arduino recibe primero RX y luego TX: GPIO14, GPIO13 en este proyecto.
    serial_.begin(baud, SERIAL_8N1, rxPin_, txPin_);
}

void DisplayCOM11441::clear() {
    serial_.write(static_cast<uint8_t>(0x76));
}

void DisplayCOM11441::setBrightness(uint8_t value) {
    serial_.write(static_cast<uint8_t>(0x7A));
    serial_.write(value);
}

void DisplayCOM11441::showText(const char* text, uint8_t decimals) {
    if (!text) {
        clear();
        return;
    }
    // El punto no ocupa un carácter. Al mostrar batería se apagan los puntos.
    serial_.write(static_cast<uint8_t>(0x77));
    serial_.write(decimals);
    // Cursor al primer dígito, sin apagar el display entre valores.
    // Siempre escribe 4 caracteres: borra dígitos sobrantes con espacios.
    serial_.write(static_cast<uint8_t>(0x79));
    serial_.write(static_cast<uint8_t>(0x00));
    bool fin = false;
    for (int i = 0; i < 4; ++i) {
        if (!fin && text[i] == '\0') fin = true;
        serial_.write(static_cast<uint8_t>(fin ? ' ' : text[i]));
    }
}

void DisplayCOM11441::showNumber(int value) {
    if (value > 9999) value = 9999;
    else if (value < -999) value = -999;
    char buffer[6];
    snprintf(buffer, sizeof(buffer), "%4d", value);
    showText(buffer);
}

void DisplayCOM11441::showBatteryPercentage(int percentage) {
    if (percentage < 0) percentage = 0;
    else if (percentage > 100) percentage = 100;
    // Se muestra el número; el símbolo % no es un carácter estándar.
    showNumber(percentage);
}

void DisplayCOM11441::showSpeedTenths(int tenths) {
    // Tres cifras y el guion: 271- con el punto del segundo dígito = 27.1-.
    // El rango visible con este formato es 0.0 a 99.9 km/h.
    if (tenths < 0 || tenths > 999) {
        showText("----");
        return;
    }
    char buffer[5] = {
        tenths >= 100 ? static_cast<char>('0' + tenths / 100) : ' ',
        static_cast<char>('0' + (tenths / 10) % 10),
        static_cast<char>('0' + tenths % 10), '-', '\0'
    };
    showText(buffer, 0x02);
}
