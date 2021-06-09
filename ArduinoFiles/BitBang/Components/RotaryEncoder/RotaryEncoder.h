/*
RotaryEncoder.h - Library for Rotary Encoders
*/

#ifndef RotaryEncoder_h
#define RotaryEncoder_h

#include "Arduino.h"
#include "Component.h"

class RotaryEncoder: public Component
{
    public:
        RotaryEncoder() {}
        RotaryEncoder(int pin1, int pin2, uint8_t id);
        void getValues(uint8_t *arr);
    private:
        int _pin0;
        int _pin1;
};

#endif
