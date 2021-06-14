#include "Arduino.h"
#include "CRC8.h"

CRC8::CRC8(uint8_t divisor) {
    _divisor = divisor;
    createLookupTable();
}


void CRC8::createLookupTable() {
    for (int divident = 0; divident < 256; divident++) {
        uint8_t cur = divident;
        for (int b = 0; b < 8; b++) {
            if ((cur & 0x80) != 0) {
                cur = (cur << 1) ^ _divisor;
            }
            else {
                cur <<= 1;
            }
        }
        _lookupTable[divident] = cur;
    }
}

uint8_t CRC8::getCRC8(uint8_t *data, int n) {
    uint8_t crc = 0;
    for (int i = 0; i < n; i++) {
        uint8_t b = data[i] ^ crc;
        crc = _lookupTable[b];
    }
    return crc;
}