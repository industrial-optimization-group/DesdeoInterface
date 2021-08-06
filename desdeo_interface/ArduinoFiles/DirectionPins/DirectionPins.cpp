/*
    DirectionPins.cpp - Library for handling direction pins
*/

#include "Arduino.h"
#include "DirectionPins.h"

DirectionPins::DirectionPins(uint8_t top, uint8_t right, uint8_t bottom, uint8_t left) {
    _pins[0] = top;
    _pins[1] = right;
    _pins[2] = bottom;
    _pins[3] = left;
    setPinsInput();
}

/*
 * Function: setPinsInput 
 * --------------------
 * Sets all the direction pins to input pullup
 */
void DirectionPins::setPinsInput() {
  for (int i = 0; i < 4; ++i) {
      pinMode(_pins[i], INPUT_PULLUP);
    }
}

/*
 * Function: setPinsInput 
 * --------------------
 * Sets all the desired direction pins to input pullup
 *  Used after the configuration state to determine if a node has disconnected
 * 
 * dirs: The directions to be set as input pullup
 * n: The count of directions
 */
void DirectionPins::setPinsInput(uint8_t *dirs, uint8_t n) {
   for (int i = 0; i < n; ++i) {
       uint8_t dir = dirs[i];
       pinMode(_pins[dir], INPUT_PULLUP);
   }
}

/*
 * Function: setPinLow 
 * --------------------
 * Pulls a desired pin low
 *  Used in checking surroundings in the configuration state
 * 
 * dir: The pin to be pulled low
 */
void DirectionPins::setPinLow(uint8_t dir) {
    pinMode(_pins[dir], OUTPUT);
    digitalWrite(_pins[dir], LOW);
}

/*
 * Function: isAnyPinLow 
 * --------------------
 * Checks whether any pin is reading low
 * 
 * Returns: True if any of the 4 pins is low. False if all the pins read high
 */
bool DirectionPins::isAnyPinLow() {
    for (int i = 0; i < 4; ++i) {
        if (digitalRead(_pins[i]) == LOW) return true;
    }
    return false;
}

/*
 * Function: isPinLow 
 * --------------------
 * Checks whether a given pin reads low
 * 
 * dir : The pin to be checked
 * 
 * Returns: True if the pin reads low. Otherwise false
 */
bool DirectionPins::isPinLow(uint8_t dir) {
    return digitalRead(_pins[dir]) == LOW;
}