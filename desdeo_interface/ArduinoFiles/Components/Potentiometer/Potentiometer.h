/*
    Potentiometer.h - Library for a potentiometer component
*/

#ifndef Potentiometer_h
#define Potentiometer_h

#include "Component.h"
#include "Arduino.h"
#include <ADS1X15.h>
class Potentiometer: public Component
{
public:
    Potentiometer() {}
    Potentiometer(uint8_t analogPin, uint8_t id);
    float getValue(ADS1115 adc); // Interupt kind of thing could be added
    float getValue() {return 0;} // Could be removed
    void setBounds(float max, float min);

private:
    uint16_t filter(uint16_t value);
    float scale(float value, float min, float max);
    int _pin;
    float _min = 0;
    float _max = 0;
    long _lastRead = 0;
};

#endif
