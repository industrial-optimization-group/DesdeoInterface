/*
RotaryEncoder.h - Library for a rotary encoder
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
        void setBounds(double max, double min, double stepSize);
        double getValue();
    private:
        int _pin0;
        int _pin1;
        double _stepSize = 1;
        double _min = 0;
        double _max = 100;
        bool _prevState;
};

#endif
