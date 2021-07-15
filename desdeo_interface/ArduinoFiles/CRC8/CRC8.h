/*
CRC.h - Library for 8bit cyclic redundancy checking
*/

#ifndef CRC8_h
#define CRC8_h

#include "Arduino.h"

class CRC8
{
public:
    CRC8(uint8_t divisor);
    uint8_t getCRC8(uint8_t *data, int n);

private:
    uint8_t _divisor;
    //uint8_t _lookupTable[256];
    //void createLookupTable();

};

#endif
