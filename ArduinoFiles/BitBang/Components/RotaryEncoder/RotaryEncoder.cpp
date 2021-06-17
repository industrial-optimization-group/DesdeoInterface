#include "Arduino.h"
#include "Component.h"
#include "RotaryEncoder.h"

RotaryEncoder::RotaryEncoder(int pin1, int pin2, uint8_t id): Component(id, 'R') {
    pinMode(pin1, INPUT);
    pinMode(pin2, INPUT);
    _pin0 = pin1;
    _pin1 = pin2;
    //_prevValue = 10 * digitalRead(_pin0) + digitalRead(_pin1); // 11, 10, 1, 0
    _prevValue = 0;
    _prevState = digitalRead(_pin0);
};

void RotaryEncoder::getValues(uint8_t *arr) {
    arr[0] = digitalRead(_pin0);
    arr[1] = digitalRead(_pin1);
    int newValue = 10 * digitalRead(_pin0) + digitalRead(_pin1);
    _hasChanged = (_prevValue != newValue);
    _prevValue = newValue;
};

uint16_t RotaryEncoder::getValue() {
    bool p1 = digitalRead(_pin0);
    bool p2 = digitalRead(_pin1);
    if (p1 != _prevState) {
        _prevValue += (p1 != p2) ? -1 : 1;
        _hasChanged = true;
    }
    else {_hasChanged = false;}
    _prevState = p1;
    return _prevValue;
}