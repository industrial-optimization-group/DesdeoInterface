/*
Component.h - Library for a base component
*/

#ifndef Component_h
#define Component_h

#include "Arduino.h"

class Component
{
public:
    Component() {}
    Component(uint8_t id, char type);
    bool hasChanged();
    uint8_t getId();
    char getType();
    virtual double getValue() = 0;

protected:
    bool _hasChanged;
    double _prevValue;

private:
    uint8_t _id;
    char _type;
};

#endif
