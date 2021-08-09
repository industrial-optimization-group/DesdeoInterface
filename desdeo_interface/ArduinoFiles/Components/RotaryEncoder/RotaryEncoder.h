/*
    RotaryEncoder.h - Library for a rotary encoder component
*/

#ifndef RotaryEncoder_h
#define RotaryEncoder_h

#include "Arduino.h"
#include "Component.h"

class RotaryEncoder: public Component
{
    public:
        RotaryEncoder() {}
        RotaryEncoder(uint8_t pin1, uint8_t pin2, uint8_t id);
        void setBounds(float max, float min, float stepSize);
        float getValue();

    private:
        uint8_t _pin0;
        uint8_t _pin1;
        float _stepSize = 1;
        float _min = 0;
        float _max = 100;
        bool _prevState;
};

#endif
