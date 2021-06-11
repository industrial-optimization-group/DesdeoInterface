#include <Potentiometer.h>
#include <Button.h>
#include <RotaryEncoder.h>
#include <PJONSoftwareBitBang.h>
#include <ArduinoJson.h>

PJONSoftwareBitBang bus(0);
StaticJsonDocument<512> doc; //512 is the RAM allocated to this document.
JsonArray rotary = doc["master"].createNestedArray("Rotary");

const int communicationPin = 12;
const int outputPin = 11;
const int inputPin = 12;

unsigned long startMillis;
unsigned long currentMillis;

// Make a own class for all component making and checking
// Modulemaker or something

// Have this serial sending as it's own class also

// 'Trick' the master to think the components connected to it are coming from a module
Button buttons[2] = {Button(2,0), Button(3,1)};
RotaryEncoder rotEncoder = RotaryEncoder(8,9,0);

void receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info) {
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
//    writeJsonToSerial();
//    uint8_t nodeComponentCount[1] = {getComponentCount(id)};
//    bus.send_packet(id, nodeComponentCount, 1);
    writeJsonToSerial();
};

void writeJsonToSerial(){
   serializeJson(doc, Serial);
   Serial.println();
}

uint8_t getComponentCount(int id) {
    uint8_t componentCount = 0;
    JsonObject node = doc[String(id)].as<JsonObject>();
    for (JsonPair kv : node) {
      String compType = kv.key().c_str();
      componentCount += doc[String(id)][compType].size();
    }
    return componentCount;
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {;}
  bus.strategy.set_pins(12, 11);
  bus.begin();
  bus.set_receiver(receiver_function);
  startMillis = millis();
};

void loop() {
  uint8_t values[2];
  rotEncoder.getValues(values);
  rotary[0] = values[0];
  rotary[1] = values[1];

  doc["master"]["Accept"] = buttons[0].getValue();
  doc["master"]["Decline"] = buttons[1].getValue();
  if (rotEncoder.hasChanged() || buttons[0].hasChanged() || buttons[1].hasChanged()) { writeJsonToSerial(); }
  bus.receive(1000);
};
