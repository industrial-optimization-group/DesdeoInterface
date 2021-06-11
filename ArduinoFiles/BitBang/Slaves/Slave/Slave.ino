#include <Potentiometer.h>
#include <RotaryEncoder.h>
#include <Button.h>
#include <PJONSoftwareBitBang.h>

const int outputPin = 12; //receiving
const int inputPin = 11; //sending
const int communicationPin = 12;
const int master = 0;

unsigned long startMillis;
unsigned long currentMillis;

// Adjust these values depending on the module
uint8_t id = 194; // Make sure each node has different id

const int potCount = 1;
const int rotCount = 1;
const int buttonCount = 0;

const int potPins[potCount] = {A5};
const int bPins[buttonCount] = {};
const int rotPins[rotCount][2] = {{8,9}};

// Until here

Potentiometer pots[potCount];
RotaryEncoder rots[rotCount];
Button buttons[buttonCount];

PJONSoftwareBitBang bus(id);

void sendPotData(int potIndex, uint16_t value) {
      Potentiometer pot = pots[potIndex];
      char type = pot.getType();
      uint8_t potId = pot.getId();
      uint8_t content[5] = {id, type, potId, (uint8_t)(value >> 8), (uint8_t)(value & 0xFF)};
      sendToMaster(content, 5); 
      pots[potIndex] = pot;
}

void sendRotData(int rotIndex, uint8_t values[2]) {
      RotaryEncoder rot = rots[rotIndex];
      uint8_t content[5] = {id, rot.getType(), rot.getId(), values[0], values[1]};
      sendToMaster(content, 5); 
      rots[rotIndex] = rot;
}

void sendButtonData(int bIndex, uint8_t value) {
      Button button = buttons[bIndex];
      char type = button.getType();
      uint8_t bId = button.getId();
      uint8_t content[4] = {id, type, bId, value};
      sendToMaster(content, 4);
      buttons[bIndex] = button;
}

void checkPots(bool forceSend = false) {
      for (int i = 0; i < potCount; i++) {
        Potentiometer pot = pots[i];
        uint16_t potValue = pot.getValue();
        pots[i] = pot;
        if (pot.hasChanged() || forceSend) {sendPotData(i, potValue);}
    }
};

void checkRots(bool forceSend = false) {
   for (int i = 0; i < rotCount; i++) {
      RotaryEncoder rot = rots[i];
      uint8_t values[2];
      rot.getValues(values);
      rots[i] = rot;
      if (rot.hasChanged() || forceSend) {sendRotData(i, values);}
    }
}

void checkButtons(bool forceSend = false) {
     for (int i = 0; i < buttonCount; i++) {
        Button button = buttons[i];
        uint8_t value = button.getValue();
        buttons[i] = button;
        if (button.hasChanged() || forceSend) {sendButtonData(i, value);}
     }
}

bool sendToMaster(uint8_t content[], int length) {
     uint16_t packet = bus.send_packet(master, content, length);
     bus.receive(10000);
     return (packet != PJON_FAIL);
}

void receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info) {
//  uint8_t compCount = payload[0];
//  forceSend = (potCount + rotCount + buttonCount != compCount); // send data if master is missing any!
}


void setup() {
   bus.strategy.set_pins(inputPin, outputPin);
   bus.begin();
   bus.set_receiver(receiver_function);

  for (int i = 0; i < potCount; i++) {
    Potentiometer pot = Potentiometer(potPins[i], i);
    pots[i] = pot;
  }

  for (int i = 0; i < rotCount; i++) {
    RotaryEncoder rot = RotaryEncoder(rotPins[i][0], rotPins[i][1], i);
    rots[i] = rot;
  }

  for (int i = 0; i < rotCount; i++) {
    Button button = Button(bPins[i], i);
    buttons[i] = button;
  }

  startMillis = millis();
  
}

void loop() {
    currentMillis = millis();
    bool forceSend = false;
    if (currentMillis - startMillis >= 3000) { //send data every 3 seconds no matter what. 
      forceSend = true;
      startMillis = millis();
    }
    checkPots(forceSend);
    checkRots(forceSend);
    checkButtons(forceSend);
};
