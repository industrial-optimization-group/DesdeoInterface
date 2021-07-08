#include "Arduino.h"
#include "Potentiometer.h"
#include "Component.h"

Potentiometer::Potentiometer(int analogPin, uint8_t id): Component(id, 'P')
{
    _pin = analogPin;
    pinMode(_pin, INPUT);
    _prevValue = analogRead(_pin);
};

// Get the analog pin value the potentiometer is connected to
uint16_t Potentiometer::getValue()
{
    uint16_t analogVal = analogRead(_pin);
    analogVal = filter(analogVal);;
    _hasChanged = (_prevValue != analogVal);
    _prevValue = analogVal;
    return analogVal;
};

// Filter the analog value
uint16_t Potentiometer::filter(uint16_t value)
{
    float a = 0.12;
    return ((a * value) + ((1-a)*_prevValue));
};
