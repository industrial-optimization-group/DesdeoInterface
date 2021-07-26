#include "Arduino.h"
#include "Potentiometer.h"
#include "Component.h"

Potentiometer::Potentiometer(int analogPin, uint8_t id): Component(id, 'P')
{
    _pin = analogPin;
};

// Get the analog pin value the potentiometer is connected to
double Potentiometer::getValue()
{
    uint16_t analogVal = analogRead(_pin);
    analogVal = filter(analogVal);;
    _hasChanged = (_prevValue != analogVal);
    _prevValue = analogVal;
    return scale(analogVal, _min, _max);
};

// Filter the analog value
uint16_t Potentiometer::filter(uint16_t value)
{
    float a = 0.12;
    return ((a * value) + ((1-a)*_prevValue));
};

double Potentiometer::scale(double value, double min, double max) 
{
    if (max <= min) return value;
    return min + (value / 1023)*(max-min);
}


void Potentiometer::setBounds(double min, double max)
{
    if (max <= min) return;
    _min = min;
    _max = max;
}

void Potentiometer::activate()
{
    pinMode(_pin, INPUT);
    _prevValue = analogRead(_pin);
}