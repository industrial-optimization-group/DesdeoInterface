#include <PJONSoftwareBitBang.h>
#include <ArduinoJson.h>


PJONSoftwareBitBang bus(0);
StaticJsonDocument<512> doc; //512 is the RAM allocated to this document.

void receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info) {
  /* Make use of the payload before sending something, the buffer where payload points to is
     overwritten when a new message is dispatched */
     String t;
     t += char(payload[0]);
     uint8_t id = (uint8_t)(payload[1]);
     uint16_t value = 0;
    if (t == "B") {
      //Serial.print("Button ID: ");
      //Serial.print(id);
      value = (uint16_t)(payload[2]);
      //Serial.print(". Value: ");
      //Serial.println(value);
    }
    else if (t == "P") {
      //Serial.print("Potentiometer ID: ");
      //Serial.print(id);
      value = ((uint16_t)(payload[2] << 8) | (uint16_t)(payload[3] & 0xFF));
      //Serial.print(". Value: ");
      //Serial.println(value);
    }
  doc[t][String(id)] = value;
  serializeJson(doc, Serial);
  Serial.println();
};

bool changed() {
return false;
};

void setup() {
  bus.strategy.set_pin(12);
  bus.begin();
  bus.set_receiver(receiver_function);

  Serial.begin(9600);
};

void loop() {
  bus.receive(1000);
};
