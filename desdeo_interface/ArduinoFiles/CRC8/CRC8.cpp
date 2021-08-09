/*
    CRC.h - Library for 8bit cyclic redundancy checking
*/

#include "Arduino.h"
#include "CRC8.h"

CRC8::CRC8(uint8_t divisor) {
    _divisor = divisor;
   //createLookupTable();
}

/*
 * Function: createLookupTable 
 * --------------------
 * Calculates whole lookup table and saves it.
 *  Will speed up getCRC8 method as every checksum is precalculated.
 * 
 * Removed because uses quite a bit of memory.
 */
// void CRC8::createLookupTable() {
//     for (int divident = 0; divident < 256; divident++) {
//         uint8_t cur = divident;
//         for (int b = 0; b < 8; b++) {
//             if ((cur & 0x80) != 0) {
//                 cur = (cur << 1) ^ _divisor;
//             }
//             else {
//                 cur <<= 1;
//             }
//         }
//         _lookupTable[divident] = cur;
//     }
// }

/*
 * Function: getCRC8 
 * --------------------
 * Calculates the crc8 checksum
 * 
 * data: The data of which the checksum is calculated
 * n: Size of data
 *
 * returns: The crc8 checksum
 */
uint8_t CRC8::getCRC8(uint8_t *data, uint8_t n) {
    uint8_t crc = 0;
    for (int i = 0; i < n; i++) {
        crc = data[i] ^ crc;
        for (int b = 0; b < 8; b++) {
            if ((crc & 0x80) != 0) {
                crc = (crc << 1) ^ _divisor;
            }
            else {
                crc <<= 1;
            }
        }
    }
    return crc;
}