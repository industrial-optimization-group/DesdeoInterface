#include <RotaryEncoder.h>
#include <Potentiometer.h>
#include <PJONSoftwareBitBang.h>

uint8_t id = 1; // Make sure each node has different id

const int communicationPin = 12;
const int master = 0;

// Adjust these values depending on the module
const int potCount = 1;
const int rotCount = 0;
const int buttonCount = 0;

const int potPins[potCount] = {A5};
const int bPins[buttonCount] = {};
const int rotPins[rotCount][2] = {};//{{8,9}};

// Until here

Potentiometer pots[potCount];
RotaryEncoder rots[rotCount];

PJONSoftwareBitBang bus(id);

void setup() {
   bus.strategy.set_pin(communicationPin);
   bus.begin();
   Serial.begin(9600);
   
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

void checkPots() {
      for (int i = 0; i < potCount; i++) {
        Potentiometer pot = pots[i];
        uint16_t potValue = pot.getValue();
        char type = pot.getType();
        uint8_t potId = pot.getId();
        if (pot.hasChanged()) {
          uint8_t content[5] = {id, type, potId, (uint8_t)(potValue >> 8), (uint8_t)(potValue & 0xFF)};
          bus.send_packet(master, content, 5);
      }
      pots[i] = pot;
    }
};

void checkRots() {
   for (int i = 0; i < rotCount; i++) {
      RotaryEncoder rot = rots[i];
      uint8_t values[2];
      rot.getValues(values);
      if (rot.hasChanged()) {
        uint8_t content[5] = {id, rot.getType(), rot.getId(), values[0], values[1]};
        bus.send_packet(master, content, 5);
      }
      rots[i] = rot;
    }
}

void checkButtons() {
  return;
}
