/*
Button.h - A library for a button component
*/

#ifndef Button_h
#define Button_h

#include "Component.h"
#include "Arduino.h"

class Button: public Component
{
public:
    Button() {}
    Button( int pin, uint8_t id);
    uint8_t getValue();

private:
    int _pin;
};

#endif
