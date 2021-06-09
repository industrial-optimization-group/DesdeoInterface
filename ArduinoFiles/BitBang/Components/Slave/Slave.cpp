#include "Arduino.h"
#include <PJONSoftwareBitBang.h>
#include "Slave.h"

Slave::Slave(uint8_t id, int pin) {
    _masterId = 0;
    _pin = pin;
    _id = id;
    bus.strategy.set_pin(_pin);
    bus.begin();
};

void Slave::send(uint8_t componentId, uint8_t data[], int dataLength) {
    int k = 2+dataLength;
    uint8_t pContent[k] = {componentId, _id};
    for (int i = 2; i < k; i++) {
        pContent[i] = data[i-2];
    }
    bus.send(_masterId, pContent, k);
};