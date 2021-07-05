#include <ConfiguationFinder.h>
#include <CRC8.h>
#include <Potentiometer.h>
#include <Button.h>
#include <RotaryEncoder.h>
#define PJON_INCLUDE_MAC
#include <PJONSoftwareBitBang.h>

// Whenever a value changes a this struct is send to master
struct Data
{
  uint8_t nodeId;
  uint16_t value;
  uint8_t id;
  char type;
};

// Is used for the stack in the configuration stage
typedef struct
{
  uint8_t nodeId;
  uint8_t dir = 0;
} stackPairs;

// more than 150 will cause memory issues
const uint8_t maxNodes = 128;

// For configuration
bool interfaceReady = false;
bool packetReceived = false;
uint8_t nextId = 1; // upto 253, Do not start from 0 as that is for broadcasting
uint8_t stackTop = 0; // master at bottom
String currentPos = ""; // Keep track of the current position we're in the configuration stage


// For checking if nodes are still alive
bool directionsToCheck[4];
unsigned long lastAlive[maxNodes]; // index = id - 1
unsigned long lastCheck;
const int maxDeadTime = 5000; // Milliseconds

// For cyclic redundancy check
const uint8_t crcKey = 7; // This key has to be same on both sides
CRC8 crc = CRC8(crcKey);

// This module handles lighting up pins
ConfigurationFinder cf = ConfigurationFinder(4, 5, 6, 7); // UP, RIGHT, DOWN, LEFT

// PJON Communication
PJONSoftwareBitBang bus(254); // 254 is the master id, each slave should know this
const int outputPin = 11;
const int inputPin = 12;

// Masters components
Button buttons[2] = {Button(2, 0), Button(3, 1)};
RotaryEncoder rotEncoder = RotaryEncoder(8, 9, 0);

// This function is called whenever data is received
// Other than component data should be send as a array so that first index is the type of the msg
void receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &info)
{
  if (char(payload[0]) == 'C') 
  { // OK Slave received data in configuration state and send component info
    packetReceived = true;
    String info = "";
    info += nextId;
    info += ":";
    for (int i = 1; i < length; ++i)
    { // Print all send info to serial seperated by spaces
      info += payload[i];
      info += ":";
    }
    info += (currentPos);
    info += " ";
    toSerialWithCRC(info);
  }
  else if (char(payload[0]) == 'A' && length == 2 && interfaceReady)
  { // Slave sends an alive message
    uint8_t nodeId = payload[1];
    Serial.print(nodeId);
    Serial.println(" IS ALIVE");
    lastAlive[nodeId - 1] = millis(); // Update last alive time for the slave
  }
  else if (char(payload[0]) == 'H' && interfaceReady)
  {
    interfaceReady = false;
    Serial.println("Slave connected");
    runConfiguration();
  }
  else if (char(payload[0]) == 'D' && interfaceReady) {
    Serial.println("Slave disconnected");
  }
  else if (interfaceReady)
  { // Slave is sending data
    Data data;
    memcpy(&data, payload, sizeof(data));
    lastAlive[data.nodeId - 1] = millis(); // Must be alive since sending data
    String dataS = dataToString(data);
    toSerialWithCRC(dataS);
  }
};

void toSerialWithCRC(String s) {
   uint8_t data[s.length() + 1];
   s.getBytes(data, s.length() + 1);
   uint8_t crc8 = crc.getCRC8(data, s.length());
   Serial.print(s); Serial.print(" ");
   Serial.println(crc8);
}

// Write data to serial with the crc8 checksum, each dataentry seperated by a colon ':'
// and crc checksum at the end, seperated with a space ' '
String dataToString(Data data)
{
  String dataS = "";
  dataS += data.nodeId;
  dataS += ":";
  dataS += data.type;
  dataS += ":";
  dataS += data.id;
  dataS += ":";
  dataS += data.value;
  return dataS;
}

void runConfiguration() 
{
  // Make sure base values are correct, incase running configuration again
  bus.send_packet_blocking(PJON_BROADCAST, "F", 1); // Make sure slaves also reset their attributes
  interfaceReady = false;
  nextId = 1;
  stackTop = 0;
  
  bus.send_packet_blocking(PJON_BROADCAST, "L", 1);
  Serial.println("Sent L broadcast");
  
  stackPairs stack[maxNodes]; // Use this array like a stack.
  configuration(stack);
  
  interfaceReady = true;
  cf.setPinsInput();
  // Let the slaves know that configuration is done
  bus.send_packet_blocking(PJON_BROADCAST, "S", 1); 
  lastCheck = millis();
}

// Initial configuration
// stack: Initally this is a array where all pairs are equal to (0,0)
// The stack will be updated as the configuration continues
void configuration(stackPairs stack[maxNodes])
{
  uint8_t dir = stack[stackTop].dir;
  updateCurrentPos(stack); // Set a position for a potential slave
  if (dir == 4) // Invalid direction
  {
    if (stackTop == 0)
    { // Master checked all its directions
      Serial.println("Configuration done");
    }
    else
    { // Slave has checked all its directions
      stackTop--;
    }
    return;
  }

  // Direction is valid
  if (stackTop == 0)
  { // master at top
    cf.setPinHigh(dir);
  }
  else
  { //  slave from stack
    uint8_t content[2] = {'N', dir};
    bus.send_packet_blocking(stack[stackTop].nodeId, content, 2); // send instruction to slave
    // The slave will react by setting the corresponding pin to high
  }

  uint16_t response = bus.receive(50000); // 0.05 seconds
  if (response == PJON_ACK)
  { // Someone received the data. The receiver should have the isReceiver set to True now
    uint8_t content2[2] = {'I', nextId};
     bus.send_packet_blocking(PJON_BROADCAST, content2, 2); // Send the id to the slave
      if (stackTop == 0) {
        directionsToCheck[dir] = true;
      } 
      else {
        uint8_t content1[2] = {'D', dir};
        bus.send_packet_blocking(stack[stackTop].nodeId, content1, 2);
      }
    
    stack[++stackTop].nodeId = nextId++;
    stack[stackTop].dir = 0;
  
    configuration(stack);
  }
  stack[stackTop].dir = dir + 1;
  configuration(stack);
}

// Update the current position we are checking by going each direction in the stack
// Starting from the master
void updateCurrentPos(stackPairs stack[maxNodes])
{
  currentPos = ""; // Reset
  for (int i = 0; i <= stackTop; i++)
  {
    currentPos += (stack[i].dir);
  }
}

int power(int base, int exponent)
{
  int p = 1;
  for (int i = 0; i < exponent; ++i)
  {
    p *= base;
  }
  return p;
}


// Check if a node has disconnected
int8_t hasNodeDisconnected()
{
  for (int i = 1; i < nextId; ++i)
  {
    //if (nodes[i].location == 0) return true; // All nodes checked
    if (millis() - lastAlive[i - 1] > maxDeadTime)
      return i;
  }
  return -1;
}

int8_t checkNodes() {
  for (int i = 0; i < 4; ++i) {
    if (directionsToCheck[i]) {
      if (!cf.isPinHigh(i)) return i;
    }
  }
  return -1;
}



void setup()
{
  Serial.begin(9600);
  cf.setPinsInput();
  bus.strategy.set_pins(inputPin, outputPin);
  bus.begin();
  bus.set_receiver(receiver_function);
  bus.send_packet_blocking(PJON_BROADCAST, "F", 1);
};

void loop()
{
  if (!interfaceReady)
  {
    if (Serial.available() > 0)
    {
      byte b = Serial.read();
      if (char(b) == 'R')
      { // Ready on the other side
        runConfiguration();
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
  if (rotEncoder.hasChanged())
  {
    Data data;
    data.id = 0;
    data.type = rotEncoder.getType();
    data.nodeId = rotEncoder.getId();
    data.value = rotVal;
    String s = dataToString(data);
    toSerialWithCRC(s);
  }

  if (buttons[0].hasChanged())
  {
    Data data;
    data.id = 0;
    data.type = buttons[0].getType();
    data.nodeId = buttons[0].getId();
    data.value = acceptVal;
    String s = dataToString(data);
    toSerialWithCRC(s);
  }

  if (buttons[1].hasChanged())
  {
    Data data;
    data.id = 0;
    data.type = buttons[1].getType();
    data.nodeId = buttons[1].getId();
    data.value = declineVal;
    String s = dataToString(data);
    toSerialWithCRC(s);
  }

  if (Serial.available() > 0)
  {
    byte b = Serial.read();
    if (char(b) == 'Q')
    {
      interfaceReady = false;
      Serial.println("STOP");
      bus.send_packet_blocking(PJON_BROADCAST, "F", 1);
      stackTop = 0;
      nextId = 1;
      return;
    }
  }
  bus.receive(1500);
  int8_t disconnectedNode = checkNodes();
  if (disconnectedNode >= 0) {
    directionsToCheck[disconnectedNode] = false;
    Serial.println("Node disconnected");
    Serial.print("Position ");
    Serial.print(disconnectedNode);
    Serial.println("From master");
  }
//  if (millis() - lastCheck > maxDeadTime)
//  {
//    lastCheck = millis();
//    int8_t disconnected = hasNodeDisconnected();
//    if (disconnected > 0)
//    {
//      Serial.print("Node with id");
//      Serial.print(disconnected);
//      Serial.println(" disconnected");
//    }
//  }
};
