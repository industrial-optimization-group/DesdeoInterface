/*
    Button.h - A library for a button component used in the physical interface
*/

#ifndef Button_h
#define Button_h

#include "Component.h"
#include "Arduino.h"

class Button: public Component
{
public:
    Button() {}
    Button(uint8_t pin, uint8_t id);
    float getValue();

private:
    uint8_t _pin;
};

#endif
