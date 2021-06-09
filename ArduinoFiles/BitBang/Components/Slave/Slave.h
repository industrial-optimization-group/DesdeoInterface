/*
A base slave class for all other slaves
*/

#ifndef Slave_h
#define Slave_h

#include "Arduino.h"
#include <PJONSoftwareBitBang.h>

class Slave
{
    public:
        Slave(uint8_t id, int pin);
        void send(uint8_t componentId, uint8_t data[], int dataLength);
    private:
        //bool hasChanged( );
        PJONSoftwareBitBang bus;
        int _pin;
        uint8_t _id;
        uint8_t _masterId;
};

#endif
