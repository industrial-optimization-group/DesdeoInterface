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
      pinMode(_pins[i], INPUT);
    }
}

void ConfigurationFinder::setPinHigh(direction dir) {
    setPinsInput();
    pinMode(_pins[dir], OUTPUT);
    digitalWrite(_pins[dir], HIGH);
}

bool ConfigurationFinder::isAnyPinHigh() {
    for (int i = 0; i < 4; ++i) {
        if (digitalRead(_pins[i]) == HIGH) return true;
    }
    return false;
}