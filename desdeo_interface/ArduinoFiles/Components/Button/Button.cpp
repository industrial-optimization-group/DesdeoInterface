#include "Arduino.h"
#include "Button.h"
#include "Component.h"

Button::Button(int pin, uint8_t id): Component(id, 'B') {
    _pin = pin;
}

//Get the value of the pin the button is connected to
double Button::getValue() {
    uint8_t value = !digitalRead(_pin);
    _hasChanged = (_prevValue != value);
    _prevValue = value;
    return value;
}

void Button::activate() 
{
    pinMode(_pin, INPUT_PULLUP);
    _prevValue = !digitalRead(_pin);
}