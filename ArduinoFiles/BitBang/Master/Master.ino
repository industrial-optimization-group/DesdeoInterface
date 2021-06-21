#include <ConfiguationFinder.h>
#include <CRC8.h>
#include <Potentiometer.h>
#include <Button.h>
#include <RotaryEncoder.h>
#define PJON_INCLUDE_MAC
#include <PJONSoftwareBitBang.h>
#include <ArduinoJson.h>

bool interfaceReady = false;
uint8_t nextId = 1;
uint8_t stack[32][2]; // Use this array like a stack, maybe implement a stack at some point
int8_t stackTop = 0; // master at bottom
int positions[32];

const uint8_t crcKey = 7;
CRC8 crc = CRC8(crcKey); // Crc4 vs paritycheck

struct Data {
  uint16_t value;
  uint8_t id;
  char type;
};

ConfigurationFinder cf = ConfigurationFinder(4,5,6,7); // UP, RIGHT, DOWN, LEFT

StaticJsonDocument<256> doc; //256 is the RAM allocated to this document.

PJONSoftwareBitBang bus;
const int masterId = 254;
const int communicationPin = 12;
const int outputPin = 11;
const int inputPin = 12;

// Make a own class for component making and checking
// Modulemaker or something

// Have this serial sending as it's own class also?

// Master could have one internal component module
Button buttons[2] = {Button(2,0), Button(3,1)};
RotaryEncoder rotEncoder = RotaryEncoder(8,9,0);

void receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &info) {

    if (char(payload[0]) == 'O') {
       Serial.println("Received from slave"); 
       return;
    }
    else {
      String id;
      for (int i = 0; i < 6; ++i) {
        id += info.tx.mac[i];
      }
      Data data;
      memcpy(&data, payload, sizeof(data));
      doc[id][String(data.type)][String(data.id)] = data.value;
      writeJsonToSerial();
    }
};

uint8_t getComponentCount(String id) {
    uint8_t componentCount = 0;
    JsonObject node = doc[id].as<JsonObject>();
    for (JsonPair kv : node) {
      String compType = kv.key().c_str();
      componentCount += doc[id][compType].size();
    }
    return componentCount;
}

void writeJsonToSerial(){
   String json;
   serializeJson(doc, json);
   int len = json.length() + 1;
   uint8_t data[len];
   json.getBytes(data, len);
   uint8_t crc8 = crc.getCRC8(data, len - 1); // getBytes -> last is nul
   Serial.print(json); // print data
   Serial.println(crc8); // Add crc checksum 
}

void configuration() {
    uint8_t dir = stack[stackTop][1];
    if (dir == 4) {
      if (stackTop == 0) { // Master checked all its directions
        Serial.println("Configuration done");
        for (int i = 0; i < 32; ++i) {
          if ( i == 0 || positions[i] > 0) {
            Serial.print("Node with id: ");
            Serial.print(i);
            Serial.print(" is in position: ");
            String pos = getPosition(positions[i]);
            for (int i = pos.length()-2; i >= 0; i--){
              if (pos[i] == '0') Serial.print("UP ");
              else if (pos[i] == '1') Serial.print("RIGHT ");
              else if (pos[i] == '2') Serial.print("DOWN ");
              else Serial.print("LEFT ");
            }
            Serial.println();
          }
        }
      } else { // Slave has checked all its directions
        stackTop--;
 
      }
      return;
    }
    
    if (stackTop == 0){ // master at top
      Serial.print("Master light up ");
      Serial.println(dir);
      cf.setPinHigh(dir);
    }
    else { //  slave from stack
         Serial.print(stack[stackTop][0]);
         Serial.println(" Slave light up");
         uint8_t content[2] = {'N', dir};
         bus.send_packet_blocking(stack[stackTop][0], content, 2); // send instruction to slave
    }

    
    uint16_t response = bus.receive(500000); // 0.5 seconds
    if (response == PJON_ACK) { // Someone received the data
      Serial.println("new slave");
      positions[nextId] = givePosition();
      uint8_t content[2] = {'I', nextId};
      stack[++stackTop][0] = nextId++;
      stack[stackTop][1] = 0;
      bus.send_packet_blocking(PJON_BROADCAST, content, 2);
      configuration();
    }
     stack[stackTop][1] = dir + 1;
     configuration();
}


// Directions are 4 base so convert the 4 base direction to decimal
// Assuming that up is reserver for usb, so 0 1 cant happen -> 0 0 1 cant happen -> no overlaps
// Example: right, up, left -> 1 0 3 -> 19
// If is not -> default right movement 
// (right), up, left, right -> 1 0 3 1
int givePosition() {
  int pos = power(4, (stackTop + 1));
  for (int i = 0; i <= stackTop; i++) {
    pos += stack[i][1] * power(4, (stackTop - i));
  }
  return pos;
}

int power(int base, int exponent) {
  int p = 1;
  for(int i = 0; i < exponent; ++i) {
    p *= base;
  }
  return p;
}

String getPosition(int pos) {
   String directions = "";
   int left = pos / 4;
   uint8_t remainder = pos % 4;
   directions += remainder;
   while (left > 0) {
    remainder = left % 4;
    directions += remainder;
    left = left / 4;
   }
   return directions;
}

void setup() {
  Serial.begin(9600);
  cf.setPinsInput();
  bus.set_id(masterId);
  bus.strategy.set_pins(inputPin, outputPin);
  bus.begin();
  bus.set_receiver(receiver_function);
};

void loop() {
  if (!interfaceReady) {
    if (Serial.available() > 0) {
      byte b = Serial.read();
      if (char(b) == 'R') {
        bus.send_packet_blocking(PJON_BROADCAST,"L",1);
        Serial.println("Sent L broadcast");
        configuration();
        //interfaceReady = true;
        //bus.send_packet(PJON_BROADCAST,"S",1);
      }
    }
    return;
  }
//  uint8_t values[2];
//  rotEncoder.getValues(values);
//  rotary[0] = values[0];
//  rotary[1] = values[1];

  doc["master"]["Accept"] = buttons[0].getValue();
  doc["master"]["Decline"] = buttons[1].getValue();
  doc["master"]["Rotary"] = rotEncoder.getValue();
  
  if (rotEncoder.hasChanged() || buttons[0].hasChanged() || buttons[1].hasChanged()) { writeJsonToSerial(); }

  bus.receive(1000);
  if (Serial.available() > 0) {
      byte b = Serial.read();
      if (char(b) == 'Q') {
        interfaceReady = false; 
        bus.send_packet(PJON_BROADCAST,"Q",1); // stop
      }
    }
};
