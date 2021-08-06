/*
    DirectionPins.h - Library for handling direction pins
*/

#ifndef DirectionPins_h
#define DirectionPins_h

#include "Arduino.h"

class DirectionPins
{
public:
    DirectionPins(uint8_t t, uint8_t r, uint8_t b, uint8_t l);
    void setPinLow(uint8_t dir);
    void setPinsInput();
    void setPinsInput(uint8_t *dirs, uint8_t n);
    bool isAnyPinLow();
    bool isPinLow(uint8_t dir);

private:
    uint8_t _pins[4];
};

#endif
