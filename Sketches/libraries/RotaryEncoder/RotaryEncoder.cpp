#include "Arduino.h"
#include "RotaryEncoder.h"

RotaryEncoder::RotaryEncoder(int pin1, int pin2) {
    pinMode(pin1, INPUT);
    pinMode(pin2, INPUT);
    _value = 0;
    _prevState = 0;
    _pin0 = pin1;
    _pin1 = pin2;
};

int RotaryEncoder::UpdateValue() {
    if (digitalRead(_pin0) != _prevState) {
        _value += Direction();
    }

    _prevState = digitalRead(_pin0);
    return _value;
};

int RotaryEncoder::Direction() {
    int dir = (digitalRead(_pin0) != digitalRead(_pin1)) ?  -1 :  1;
    return dir;
};
