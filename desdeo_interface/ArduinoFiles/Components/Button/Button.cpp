/*
    Button.cpp - Library for the button component.
*/

#include "Arduino.h"
#include "Button.h"
#include "Component.h"

Button::Button(uint8_t pin, uint8_t id): Component(id, 'B') {
    _pin = pin;
    pinMode(_pin, INPUT_PULLUP);
    _prevValue = !digitalRead(_pin);
}

/*
 * Function:  getValue 
 * --------------------
 * Get the value of the button
 *
 *  returns: Either 0 (not down) or 1 (is down)
 */
float Button::getValue() {
    uint8_t value = !digitalRead(_pin);
    _hasChanged = (_prevValue != value);
    _prevValue = value;
    return value;
}