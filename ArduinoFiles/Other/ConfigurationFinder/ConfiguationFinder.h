/*
ConfigurationFinder.h - Library for determining the layout of the setup
*/

#ifndef ConfigurationFinder_h
#define ConfigurationFinder_h

#include "Arduino.h"
enum direction {top = 0, right = 1, bottom = 2, left = 3};

class ConfigurationFinder
{
public:
    ConfigurationFinder(uint8_t t, uint8_t r, uint8_t b, uint8_t l);
    void setPinHigh(direction dir);
    void setPinsInput();
    bool isAnyPinHigh();

private:
    uint8_t _pins[4];
};

#endif
