#include "Arduino.h"
#include "Led.h"

Led::Led(uint8_t redPin, uint8_t greenPin, uint8_t bluePin)
{
    _pins = {redPin, greenPin, bluePin};
    _rgb = toRgb(white)
    for (uint8_t pin: _pins)
        pinMode(pin, OUTPUT);
    _isOn = false;
}

void Led::setColor(Color color)
{
    uint8_t rgb[3] = toRgb(color);
    _setPins(rgb);
}

void Led::setColor(uint8_t red, uint8_t green, uint8_t blue)
{
    uint8_t rgb[3] = {red, green, blue};
    _setPins(rgb);
}

void Led::off()
{
    _isOn = false;
    _setPins({0,0,0});
}

void Led::on()
{
    _isOn = true;
    _setPins(_rgb);
}

void Led::toggle()
{
    if (_isOn)
        off()
    else
        on()
}

void Led::_setPins(uint8_t[3] rgb)
{
    _rgb = rgb;
    for (int i = 0; i < 3; ++i) 
        digitalWrite(_pins[i], _rgb[i]);
}