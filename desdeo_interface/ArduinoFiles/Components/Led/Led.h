/*
    Led.h - Library for displaying status codes with the help of a rgb led
*/

#ifndef Led_h
#define Led_h

#include "Arduino.h"


class Led
{
public:
    Led(uint8_t redPin, uint8_t greenPin, uint8_t bluePin);
    void setColor(uint8_t rgb[3]);
    void off();
    void on();
    void toggle();

private:
    uint8_t _pins[3];
    uint8_t _rgb[3];
    bool _isOn;
};

#endif
