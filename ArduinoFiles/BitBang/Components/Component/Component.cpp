#include "Arduino.h"
#include "Component.h"

Component::Component(uint8_t id, char type) {
    _hasChanged = false;
    _type = type;
    _id = id;
}

bool Component::hasChanged() {
    return _hasChanged;
}

uint8_t Component::getId() {
    return _id;
}

char Component::getType() {
    return _type;
}