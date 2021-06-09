#include "RotaryEncoder.h"
#include <PJONSoftwareBitBang.h>
#include <ArduinoJson.h>

PJONSoftwareBitBang bus(0);
StaticJsonDocument<512> doc; //512 is the RAM allocated to this document.
JsonArray rotary = doc["master"].createNestedArray("Rotary");
int buttons[2] = {2, 3};
//RotaryEncoder rotEncoder = RotaryEncoder(8,9);
int rotaryEncoder[2] = {8,9};

void receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info) {
  /* Make use of the payload before sending something, the buffer where payload points to is
     overwritten when a new message is dispatched */
      Serial.println("RE");

    String type;
    uint8_t id = (uint8_t)(payload[0]);
    type +=char(payload[1]);
    uint8_t componentId = (uint8_t)(payload[2]);

    if (type == "B") {
      uint16_t value = (uint16_t)(payload[3]);
      doc[String(id)][type][String(componentId)] = value;
    }
    else if (type == "P") {
      uint16_t value = ((uint16_t)(payload[3] << 8) | (uint16_t)(payload[4] & 0xFF));
      doc[String(id)][type][String(componentId)] = value;
    }
    else if (type == "R") {
      JsonArray slaveRotary = doc[type][String(id)] ;
      if (slaveRotary.isNull()) {slaveRotary = doc[String(id)][type].createNestedArray(String(componentId));}
      slaveRotary[0] = payload[3];
      slaveRotary[1] = payload[4];
    }
    else {return;}
    serializeJson(doc, Serial);
    Serial.println();
};

void setup() {
  bus.strategy.set_pin(12);
  bus.begin();
  bus.set_receiver(receiver_function);
  pinMode(buttons[0], INPUT);
  pinMode(buttons[1], INPUT);
  Serial.begin(9600);
};

void loop() {
  int bpin1 = digitalRead(buttons[0]);
  int bpin2 = digitalRead(buttons[1]);
  
  int rot1 = digitalRead(rotaryEncoder[0]);
  int rot2 = digitalRead(rotaryEncoder[1]);

  rotary[0] = rot1;
  rotary[1] = rot2;

  doc["master"]["Accept"] = bpin1;
  doc["master"]["Decline"] = bpin2;
  bus.receive(1000);
};
