/*
Potentiometer.h - Library for getting input from a potentiometer
*/

#ifndef Potentiometer_h
#define Potentiometer_h

#include "Component.h"
#include "Arduino.h"

class Potentiometer: public Component
{
public:
    Potentiometer() {}
    Potentiometer(int analogPin, uint8_t id);
    uint16_t getValue();

private:
    uint16_t filter(uint16_t value);
    int _pin;
};

#endif
