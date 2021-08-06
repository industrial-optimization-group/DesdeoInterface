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
    double getValue(ADS1115 adc); // Interupt kind of thing could be added
    double getValue() {return 0;} // Could be removed
    void setBounds(double max, double min);
    void activate();
    
private:
    uint16_t filter(uint16_t value);
    double scale(double value, double min, double max);
    int _pin;
    double _min = 0;
    double _max = 0;
    long _lastRead = 0;
};

#endif
