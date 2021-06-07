/*
asd
*/

#ifndef RotaryEncoder_h
#define RotaryEncoder_h

#include "Arduino.h"

class RotaryEncoder
{
    public:
        RotaryEncoder(int pin1, int pin2);
        int UpdateValue();
    private:
        int _value;
        int _pin0;
        int _pin1;
        int _prevState;
        int Direction(int pin0, int pin1);
};

#endif
