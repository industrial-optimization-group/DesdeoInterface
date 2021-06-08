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
        void getValues(uint8_t *arr);
    private:
        int _pin0;
        int _pin1;
};

#endif
