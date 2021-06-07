#include "Arduino.h"
#include "RotaryEncoder.h"

RotaryEncoder::RotaryEncoder(int pin1, int pin2) {
    pinMode(pin1, INPUT);
    pinMode(pin2, INPUT);
    _value = 0;
    _prevState = digitalRead(pin1);
    _pin0 = pin1;
    _pin1 = pin2;
};

int RotaryEncoder::UpdateValue() {
    int pin0 = digitalRead(_pin0);
    int pin1 = digitalRead(_pin1);
    if (pin0 != _prevState) {
        _value += Direction(pin0, pin1);
    }

    _prevState = pin0;
    return _value;
};

int RotaryEncoder::Direction(int pin0, int pin1) {
    int dir = (pin0 != pin1) ?  -1 :  1;
    return dir;
};
