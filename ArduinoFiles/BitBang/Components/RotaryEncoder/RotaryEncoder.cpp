#include "Arduino.h"
#include "RotaryEncoder.h"

RotaryEncoder::RotaryEncoder(int pin1, int pin2) {
    pinMode(pin1, INPUT);
    pinMode(pin2, INPUT);
    _pin0 = pin1;
    _pin1 = pin2;
};

void RotaryEncoder::getValues(uint8_t *arr) {
    arr[0] = digitalRead(_pin0);
    arr[1] = digitalRead(_pin1);
};