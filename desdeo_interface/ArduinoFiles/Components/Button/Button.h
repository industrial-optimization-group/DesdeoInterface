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
    Button( int pin, uint8_t id);
    double getValue();
    void activate();

private:
    int _pin;
};

#endif
