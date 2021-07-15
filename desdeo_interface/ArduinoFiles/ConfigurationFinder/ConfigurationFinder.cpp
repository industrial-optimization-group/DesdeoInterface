#include "Arduino.h"
#include "ConfiguationFinder.h"

ConfigurationFinder::ConfigurationFinder(uint8_t top, uint8_t right, uint8_t bottom, uint8_t left) {
    _pins[0] = top;
    _pins[1] = right;
    _pins[2] = bottom;
    _pins[3] = left;
    setPinsInput();
}

void ConfigurationFinder::setPinsInput() {
  for (int i = 0; i < 4; ++i) {
      pinMode(_pins[i], INPUT_PULLUP);
    }
}

void ConfigurationFinder::setPinsInput(uint8_t *dirs, uint8_t n) {
   for (int i = 0; i < n; ++i) {
       uint8_t dir = dirs[i];
       pinMode(_pins[dir], INPUT_PULLUP);
   }
}


void ConfigurationFinder::setPinLow(uint8_t dir) {
    pinMode(_pins[dir], OUTPUT);
    digitalWrite(_pins[dir], LOW);
}

bool ConfigurationFinder::isAnyPinLow() {
    for (int i = 0; i < 4; ++i) {
        if (digitalRead(_pins[i]) == LOW) return true;
    }
    return false;
}

bool ConfigurationFinder::isPinLow(uint8_t dir) {
    return digitalRead(_pins[dir]) == LOW;
}