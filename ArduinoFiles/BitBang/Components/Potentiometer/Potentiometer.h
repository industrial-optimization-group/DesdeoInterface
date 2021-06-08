/*
Potentiometer.h - Library for getting input from a potentiometer
*/

#ifndef Potentiometer_h
#define Potentiometer_h

#include "Arduino.h"

class Potentiometer
{
public:
    Potentiometer(int analogPin);
    uint16_t getValue();

private:
    uint16_t filter(uint16_t value);
    int _pin;
    uint16_t _prevValue;
};

#endif
