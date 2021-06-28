#include <ConfiguationFinder.h>
#include <CRC8.h>
#include <Potentiometer.h>
#include <Button.h>
#include <RotaryEncoder.h>
#define PJON_INCLUDE_MAC
#include <PJONSoftwareBitBang.h>
//#include <ArduinoJson.h>

// thoughs about using dictionary from arduino library 

struct Data {
  uint8_t nodeId;
  uint16_t value;
  uint8_t id;
  char type;
};

typedef struct {
  uint8_t nodeId;
  uint8_t dir = 0;
} stackPairs;

//typedef struct {
//  uint8_t buttons;
//  uint8_t rotaries;
//  uint8_t potentiometers;
//  int location;
//  unsigned long lastAlive;
//} node;

const uint8_t maxNodes = 150;

// Configuration
bool interfaceReady = false;
uint8_t nextId = 1; // upto 253, Do not start from 0 as that is for broadcasting
//int positions[64]; // Index equals to the node id, except master is at 0, this could be modified so we save one int of space
uint8_t stackTop = 0; // master at bottom
String currentPos = "";
//node nodes[maxNodes]; // position, lastAlive, potCount, bCount, Rcount

// For checking if nodes are still alive
unsigned long lastAlive[maxNodes]; // index = id - 1
unsigned long lastCheck;
const int maxDeadTime = 5000; // Milliseconds

// Cyclic redundancy check
const uint8_t crcKey = 7;
CRC8 crc = CRC8(crcKey);

// This module handles lighting up pins
ConfigurationFinder cf = ConfigurationFinder(4,5,6,7); // UP, RIGHT, DOWN, LEFT

// Write json to serial
//StaticJsonDocument<128> doc; //256 is the RAM allocated to this document.

// PJON Communication
PJONSoftwareBitBang bus(254); // 254 is the master id
const int outputPin = 11;
const int inputPin = 12;

// Masters components
Button buttons[2] = {Button(2,0), Button(3,1)};
RotaryEncoder rotEncoder = RotaryEncoder(8,9,0);

// This function is called whenever data is received
// Other than component data should be send as a array so that first index is the type of the msg
void receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &info) {
    if (char(payload[0]) == 'C') { // OK Slave received data in configuration state and send component info
      Serial.print(nextId);
      Serial.print(" ");
      for (int i = 1; i < length; ++i) {
            Serial.print(payload[i]);
            Serial.print(" ");
      }
      Serial.println(currentPos);
//       nodes[nextId].potentiometers = payload[1]; // pots
//       nodes[nextId].rotaries = payload[2]; // Rots
//       nodes[nextId].buttons = payload[3]; // Buttons
    }
    else if (char(payload[0]) == 'A' && length == 2) { //Alive
      uint8_t nodeId = payload[1];
      Serial.print(nodeId);
      Serial.println(" IS ALIVE");
      lastAlive[nodeId -1] = millis();
    }
    else { // Slave is sending data
      Data data;
      memcpy(&data, payload, sizeof(data));
      lastAlive[data.nodeId-1] = millis(); // Must be alive since sending data
      printData(data);
//      doc[String(data.nodeId)][String(data.type)][String(data.id)] = data.value;
//      writeJsonToSerial();
    }
};

void printData(Data data) {
  Serial.print(data.nodeId); Serial.print(" ");
  Serial.print(data.type); Serial.print(" ");
  Serial.print(data.id); Serial.print(" ");
  Serial.println(data.value);
}

// Write the json with the crc checksum to serial
//void writeJsonToSerial(){
//   String json;
//   serializeJson(doc, json);
//   int len = json.length() + 1;
//   uint8_t data[len];
//   json.getBytes(data, len);
//   uint8_t crc8 = crc.getCRC8(data, len - 1); // getBytes -> last is nul
//   Serial.print(json); // print data
//   Serial.println(crc8); // Add crc checksum 
//}

// Initial configurations
void configuration(stackPairs stack[maxNodes]) {
    uint8_t dir = stack[stackTop].dir;
    updateCurrentPos(stack); // Set a position for a potential slave
    if (dir == 4) {
      if (stackTop == 0) { // Master checked all its directions
        Serial.println("Configuration done");
       // printNodeInfo();
      } else { // Slave has checked all its directions
        stackTop--;
      }
      return;
    }
    
    if (stackTop == 0){ // master at top
      cf.setPinHigh(dir);
    }
    else { //  slave from stack
         //Serial.print(stack[stackTop][0]);
         //Serial.println(" Slave light up");
         uint8_t content[2] = {'N', dir};
         bus.send_packet_blocking(stack[stackTop].nodeId, content, 2); // send instruction to slave
    }

    uint16_t response = bus.receive(50000); // 0.05 seconds
    if (response == PJON_ACK) { // Someone received the data and send responded
      uint8_t content[2] = {'I', nextId};
      stack[++stackTop].nodeId = nextId++;
      stack[stackTop].dir = 0;
      bus.send_packet_blocking(PJON_BROADCAST, content, 2); // Send the id to the slave
      configuration(stack);
    }
     stack[stackTop].dir = dir + 1;
     configuration(stack);
}


// Directions are 4 base so convert the 4 base direction to decimal
// Assuming that up is reserver for usb, so 0 1 cant happen -> 0 0 1 cant happen -> no overlaps
// Example: right, up, left -> 1 0 3 -> 19
// If is not -> default right movement 
// (right), up, left, right -> 1 0 3 1
void updateCurrentPos(stackPairs stack[maxNodes]) {
  currentPos = ""; // Reset
//  int pos = power(4, (stackTop + 1));
  for (int i = 0; i <= stackTop; i++) {
  currentPos += (stack[i].dir);
//    pos += stack[i][1] * power(4, (stackTop - i));
  }
//  return pos;
}

// temp test function 
//void printNodeInfo() {
//   for (int i = 0; i < maxNodes; ++i) {
//    if (nodes[i].location > 0) { // has a position
//      Serial.print("Node with id: ");
//      Serial.print(i);
//      Serial.print(" is in position: ");
//      String pos = getPosition(nodes[i].location);
//      for (int j = pos.length()-2; j >= 0; j--){
//        if (pos[j] == '0') Serial.print("UP ");
//        else if (pos[j] == '1') Serial.print("RIGHT ");
//        else if (pos[j] == '2') Serial.print("DOWN ");
//        else Serial.print("LEFT ");
//      }
//      Serial.println();
//      Serial.print("PotCount: ");
//      Serial.println(nodes[i].potentiometers);
//      Serial.print("RotCount: ");
//      Serial.println(nodes[i].rotaries);
//      Serial.print("Buttoncount: ");
//      Serial.println(nodes[i].buttons);
//      Serial.println();
//    }
//  }
//}

int power(int base, int exponent) {
  int p = 1;
  for(int i = 0; i < exponent; ++i) {
    p *= base;
  }
  return p;
}

// Decimal positions back to 4 base directions
// Note: is flipped?
//String getPosition(int pos) {
//   String directions = "";
//   int left = pos / 4;
//   uint8_t remainder = pos % 4;
//   directions += remainder;
//   while (left > 0) {
//    remainder = left % 4;
//    directions += remainder;
//    left = left / 4;
//   }
//   return directions;
//}

bool areAllNodesAlive() {
  for (int i = 1; i < nextId; ++i) {
    //if (nodes[i].location == 0) return true; // All nodes checked
    if (millis() - lastAlive[i] > maxDeadTime) return false;
  }
  return true;
}

void setup() {
  Serial.begin(9600);
  cf.setPinsInput();
  bus.strategy.set_pins(inputPin, outputPin);
  bus.begin();
  bus.set_receiver(receiver_function);
};

void loop() {
  if (!interfaceReady) {
    if (Serial.available() > 0) {
      byte b = Serial.read();
      if (char(b) == 'R') { // Ready on the other side
        bus.send_packet_blocking(PJON_BROADCAST,"L",1);
        Serial.println("Sent L broadcast");
        stackPairs stack[maxNodes]; // Use this array like a stack. Adjust stack size if a node might be more than 32 nodes away from master
        configuration(stack);
        interfaceReady = true;
        bus.send_packet_blocking(PJON_BROADCAST, "S",1); // Let the slaves know that configuration is done
        lastCheck = millis();
      }
    }
    return;
  }

  bus.receive(1500);
  
//  doc["master"]["Accept"] = buttons[0].getValue();
//  doc["master"]["Decline"] = buttons[1].getValue();
//  doc["master"]["Rotary"] = rotEncoder.getValue();
  uint16_t acceptVal = buttons[0].getValue();
  uint16_t declineVal = buttons[1].getValue();
  uint16_t rotVal = rotEncoder.getValue();
  if (rotEncoder.hasChanged()) {
    Data data;
    data.id = 0;
    data.type = rotEncoder.getType();
    data.nodeId = rotEncoder.getId();
    data.value = rotVal;
    printData(data);
  }
  
  if (buttons[0].hasChanged()) {
    Data data;
    data.id = 0;
    data.type = buttons[0].getType();
    data.nodeId = buttons[0].getId();
    data.value = acceptVal;
    printData(data);
  }
  
  if (buttons[1].hasChanged()) { 
    Data data;
    data.id = 0;
    data.type = buttons[1].getType();
    data.nodeId = buttons[1].getId();
    data.value = declineVal;
    printData(data); 
  }

  if (Serial.available() > 0) {
      byte b = Serial.read();
      if (char(b) == 'Q') {
        interfaceReady = false; 
        bus.send_packet(PJON_BROADCAST,"Q",1); // stop
      }
    }

   if (millis() - lastCheck > maxDeadTime) {
    lastCheck = millis();
    bool alive = areAllNodesAlive();
    if (not alive) {
      Serial.println("A node disconnected");
    }
   }
   bus.receive(1500);
};
