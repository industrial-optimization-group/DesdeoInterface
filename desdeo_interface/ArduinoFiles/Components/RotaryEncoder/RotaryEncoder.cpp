/*
    RotaryEncoder.h - Library for a rotary encoder component
*/

#include "Arduino.h"
#include "Component.h"
#include "RotaryEncoder.h"

RotaryEncoder::RotaryEncoder(uint8_t pin1, uint8_t pin2, uint8_t id): Component(id, 'R') 
{
    _pin0 = pin1;
    _pin1 = pin2;
    _prevValue = 0;
};

/*
 * Function:  getValue 
 * --------------------
 * Reads the value of the rotary encoder
 *
 *  returns: The value of the rotary encoder
 */

double RotaryEncoder::getValue()
{
    bool p1 = digitalRead(_pin0);
    bool p2 = digitalRead(_pin1);
    if (p1 != _prevState && p1 == 0) {
        int8_t dir = ((p1 != p2) ? -1 : 1);
        double step = _stepSize * dir;
        _prevValue += step;
        if (_prevValue > _max) _prevValue = _min;
        else if (_prevValue < _min) _prevValue = _max;
        _hasChanged = true;
    }
    else {
        _hasChanged = false;
    }
    _prevState = p1;
    return _prevValue;
}

/*
 * Function:  setBounds 
 * --------------------
 * Sets value bounds and step size for the rotary encoder.
 *
 *  min: Minimum value the rotary encoder can achieve
 *  max: Maximum value the rotary encoder can achieve
 *  stepSize: How much does the value increase/decrease by one rotation
 */

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
    pinMode(_pin0, INPUT_PULLUP);
    pinMode(_pin1, INPUT_PULLUP);
    _prevState = digitalRead(_pin0);
}