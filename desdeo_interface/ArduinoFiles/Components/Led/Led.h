/*
Led.h - Library for displaying status with a led
*/

#ifndef Led_h
#define Led_h

#include "Arduino.h"

enum Color
{
    green, red, blue, white
};

uint8_t[3] toRgb(Color color)
{
    uint8_t rgb[3];
    switch (color)
    {
        case green:
            rgb = {0, 255, 0};
            break;
        case red:
            rgb = {255, 0, 0};
            break;
        case blue:
            rgb = {0,0,255};
            break;
        case white:
            rgb = {255, 255, 255};
            break;
        default:
            rgb = {0,0,0};
            break;
    }
    return rgb;
}


class Led
{
public:
    Led(uint8_t redPin, uint8_t greenPin, uint8_t bluePin);
    void setColor(Color color);
    void setColor(uint8_t red, uint8_t green, uint8_t blue);
    void off();
    void on();
    void toggle();

private:
    uint8_t _pins[3];
    uint8_t _rgb[3];
    bool _isOn;
    void _setPins(uint8_t red, uint8_t green, uint8_t blue);
};

#endif
