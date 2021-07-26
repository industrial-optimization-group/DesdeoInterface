#include "Arduino.h"
#include "Component.h"
#include "RotaryEncoder.h"

RotaryEncoder::RotaryEncoder(int pin1, int pin2, uint8_t id): Component(id, 'R') 
{
    _pin0 = pin1;
    _pin1 = pin2;
    _prevValue = 0;
};

double RotaryEncoder::getValue() 
{
    _hasChanged = false;
    return _prevValue;
}

void RotaryEncoder::update()
{
    bool p1 = digitalRead(_pin0);
    bool p2 = digitalRead(_pin1);
    if (p1 != _prevState) {
        int8_t dir = ((p1 != p2) ? -1 : 1);
        double step = _stepSize * dir;
        _prevValue += step;
        if (_prevValue > _max) _prevValue = _min;
        else if (_prevValue < _min) _prevValue = _max;
        _hasChanged = true;
        _prevState = p1;
    }
    return _prevValue;
}

void RotaryEncoder::setBounds(double min, double max, double stepSize) 
{
    if (min >= max) return;
    _min = min;
    _max = max;
    _stepSize = stepSize;
}


// This could be removed back to constructor
void RotaryEncoder::activate() 
{
    pinMode(_pin0, INPUT);
    pinMode(_pin1, INPUT);
    _prevState = digitalRead(_pin0);
}