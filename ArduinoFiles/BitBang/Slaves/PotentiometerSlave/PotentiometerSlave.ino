#include <RotaryEncoder.h>
#include <Potentiometer.h>
#include <PJONSoftwareBitBang.h>

uint8_t id = 0; // Make sure each node has different id

const int communicationPin = 12;
const int master = 0;

// Adjust these values depending on the module
const int potCount = 0;
const int rotCount = 1;
const int buttonCount = 0;

const int potPins[potCount] = {};
const int bPins[buttonCount] = {};
const int rotPins[rotCount][2] = {{8,9}};

// Until here

Potentiometer pots[potCount];
RotaryEncoder rots[rotCount];

PJONSoftwareBitBang bus(id);

void checkPots() {
      for (int i = 0; i < potCount; i++) {
      Potentiometer pot = pots[i];
      uint16_t potValue = pot.getValue();
      if (pot.hasChanged()) {
        uint8_t content[4] = {id, pot.getType(), pot.getId(), (uint8_t)(potValue >> 8), (uint8_t)(potValue & 0xFF)};
        bus.send(master, content, 4);
      }
      pots[i] = pot;
    }
};

void checkRots() {
   for (int i = 0; i < potCount; i++) {
      RotaryEncoder rot = rots[i];
      uint8_t values[2];
      rot.getValues(values);
      if (rot.hasChanged()) {
        uint8_t content[4] = {id, rot.getType(), rot.getId(), values[0], values[1]};
        bus.send(master, content, 5);
      }
      rots[i] = rot;
    }
}

void checkButtons() {
  return;
}

void setup() {
   bus.strategy.set_pin(communicationPin);
   bus.begin();
   
  for (int i = 0; i < potCount; i++) {
    Potentiometer pot = Potentiometer(potPins[i], i);
    pots[i] = pot;
  }

  for (int i = 0; i < rotCount; i++) {
    RotaryEncoder rot = RotaryEncoder(rotPins[i][0], rotPins[i][1], i);
    rots[i] = rot;
  }

    checkPots();
    checkRots();
    checkButtons();
}

void loop() {
    checkPots();
    checkRots();
    checkButtons();
};
