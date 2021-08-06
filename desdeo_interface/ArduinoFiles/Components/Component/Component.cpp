/*
    Component.cpp - Library for a base component
*/

#include "Arduino.h"
#include "Component.h"

Component::Component(uint8_t id, char type) {
    _hasChanged = false;
    _type = type;
    _id = id;
}

/*
 * Function:  hasChanged 
 * --------------------
 * Has the component value changed. _hasChanged is mainly updated in
 *  getValue() methods.
 *
 *
 *  returns: True if the value has changed else false
 */
bool Component::hasChanged() {
    return _hasChanged;
}

/*
 * Function:  getId 
 * --------------------
 * Get the id of the component
 *
 *  returns: Id of the component
 */
uint8_t Component::getId() {
    return _id;
}

/*
 * Function:  getType 
 * --------------------
 * Get the type of the component
 *
 *  returns: type of the component
 */
char Component::getType() {
    return _type;
}