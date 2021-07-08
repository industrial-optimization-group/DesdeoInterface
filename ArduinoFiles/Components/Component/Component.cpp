#include "Arduino.h"
#include "Component.h"

Component::Component(uint8_t id, char type) {
    _hasChanged = false;
    _type = type;
    _id = id;
}

// Has the component value(s) changed
bool Component::hasChanged() {
    return _hasChanged;
}

// Get the component id
uint8_t Component::getId() {
    return _id;
}

// Get the component type
char Component::getType() {
    return _type;
}