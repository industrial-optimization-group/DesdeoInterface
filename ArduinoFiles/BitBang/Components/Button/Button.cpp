#include "Arduino.h"
#include "Button.h"
#include "Component.h"

Button::Button(int pin, uint8_t id): Component(id, 'B') {
    pinMode(pin, INPUT);
    _pin = pin;
    _prevValue = digitalRead(_pin);
}

uint8_t Button::getValue() {
    uint8_t value = digitalRead(_pin);
    _hasChanged = (_prevValue != value);
    _prevValue = value;
    return value;
}