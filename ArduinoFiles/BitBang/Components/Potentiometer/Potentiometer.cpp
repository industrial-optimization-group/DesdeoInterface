#include "Arduino.h"
#include "Potentiometer.h"

Potentiometer::Potentiometer(int analogPin)
{
    if (analogPin > A6 || analogPin < A0) // depends on board
    {
        Serial.println("Invalid pin");
        return;
    }
    pinMode(analogPin, INPUT);
    _pin = analogPin;
    _prevValue = analogRead(_pin);
};

uint16_t Potentiometer::getValue()
{
    uint16_t analogVal = analogRead(_pin);
    analogVal = filter(analogVal);
    _prevValue = analogVal;
    return analogVal;
};


uint16_t Potentiometer::filter(uint16_t value)
{
    float a = 0.24;
    return (a * value) + ((1-a)*_prevValue);
};
