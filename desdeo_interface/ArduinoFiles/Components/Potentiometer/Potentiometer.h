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
    double getValue();
    void setBounds(double max, double min);

private:
    uint16_t filter(uint16_t value);
    double scale(double value, double min, double max);
    int _pin;
    double _min = 0;
    double _max = 0;
};

#endif
