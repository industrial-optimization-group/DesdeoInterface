#include <ConfiguationFinder.h>
#include <CRC8.h>
#include <Potentiometer.h>
#include <Button.h>
#include <RotaryEncoder.h>
#include <PJONSoftwareBitBang.h>
// more than 150 will cause memory issues
const uint8_t maxNodes = 128;

// Is used for the stack in the configuration stage
typedef struct
{
  uint8_t nodeId;
  uint8_t dir = 0;
} stackPair;

// Direction with nodes to check if alive
bool directionToCheck[4];

// Whenever a value changes a this struct is send to master
struct Data
{
  uint8_t nodeId;
  double value;
  uint8_t id;
  char type;
};

#define BOUNDS_SIZE 30
// Sending component bounds to node
struct Bounds 
{
  char componentType;
  uint8_t componentId;
  double minValue;
  double maxValue;
  double stepSize;
};

// For configuration
bool interfaceReady = false;
bool packetReceived = false;
uint8_t nextId = 1; // upto 253, Do not start from 0 as that is for broadcasting
uint8_t stackTop = 0; // master at bottom
String currentPos = ""; // Keep track of the current position we're in the configuration stage

// For cyclic redundancy check
const uint8_t crcKey = 7; // This key has to be same on both sides
CRC8 crc = CRC8(crcKey);

// This module handles direction pins
ConfigurationFinder cf = ConfigurationFinder(4, 5, 6, 7); // UP, RIGHT, DOWN, LEFT

// PJON Communication
PJONSoftwareBitBang bus(254); // 254 is the master id, each slave should know this
const int outputPin = 11;
const int inputPin = 12;

// Masters components
Button buttons[2] = {Button(2, 0), Button(3, 1)};
RotaryEncoder rotEncoder = RotaryEncoder(8, 9, 0);

// Communication packet ids. These could also be their own module as they are somewhat shared
const char nodeInfo = 'N';
const char nodeConnected = 'C'; 
const char nodeDisconnected = 'D';
const char componentValue = 'V';
const char start = 'S';
const char quit = 'Q';
const char reset = 'E';
const char idToNode = 'I';
const char dirInstruction = 'N';
const char csCompleted = 'O';
const char dirToCheck = 'D';
const char bounds = 'B';


// Convert Data type to string
String dataToString(Data data)
{
  String dataS = String(componentValue);
  dataS += " ";
  dataS += data.nodeId;
  dataS += ":";
  dataS += data.type;
  dataS += ":";
  dataS += data.id;
  dataS += ":";
  dataS += data.value;
  return dataS;
}

// This function is called whenever data is received
// Other than component data should be send as a array so that first index is the type of the msg
void receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &info)
{
  if (char(payload[0]) == nodeInfo) 
  { // OK: Node received data in configuration state and send component info
    packetReceived = true;
    String info = String(nodeInfo);
    info += " ";
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
  else if (char(payload[0]) == nodeConnected && interfaceReady)
  { // A node connected
    interfaceReady = false;
    runConfiguration();
  }
  else if (char(payload[0]) == nodeDisconnected && interfaceReady)
  { // A node disconnected
    uint8_t nodeId = payload[1];
    uint8_t dir = payload[2];
    disconnectedNodeToSerial(nodeId, dir);
  }
  else if (interfaceReady)
  { // Node sending data, figure out a better way
    Data data;
    memcpy(&data, payload, sizeof(data));
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


void runConfiguration() 
{
  // Make sure base values are correct, incase running configuration again
  uint8_t packet[1] = {reset};
  bus.send_packet_blocking(PJON_BROADCAST, packet, 1); // Make sure slaves also reset their attributes
  interfaceReady = false;
  nextId = 1;
  stackTop = 0;
  
  packet[0] = start;
  bus.send_packet_blocking(PJON_BROADCAST, packet, 1);
  
  stackPair stack[maxNodes]; // Use this array like a stack.
  configuration(stack);

  interfaceReady = true;
  cf.setPinsInput();
  
  // Let the slaves know that configuration is done
  packet[0] = csCompleted;
  bus.send_packet_blocking(PJON_BROADCAST, packet, 1); 
}

// Initial configuration: Assign ids dynamically to nodes,
// and find their positions
// stack: Initally this is a array where all pairs are equal to (0,0)
// The stack will be updated as the configuration continues
void configuration(stackPair stack[maxNodes])
{
  uint8_t dir = stack[stackTop].dir;
  updateCurrentPos(stack);
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
    uint8_t content[2] = {dirInstruction, dir};
    bus.send_packet_blocking(stack[stackTop].nodeId, content, 2); // send instruction to slave
    // The slave will react by setting the corresponding pin to high
  }

  uint16_t response = bus.receive(50000); // 0.05 seconds
  if (response == PJON_ACK)
  { // Someone received the data. The receiver should have the isReceiver set to True now
    uint8_t content2[2] = {idToNode, nextId};
     bus.send_packet_blocking(PJON_BROADCAST, content2, 2); // Send the id to the slave
      if (stackTop == 0) {
        directionToCheck[dir] = true;
      } 
      else {
        uint8_t content1[2] = {dirToCheck, dir};
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
void updateCurrentPos(stackPair stack[maxNodes])
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

int8_t checkNodes() {
  for (int i = 0; i < 4; ++i) {
    if (directionToCheck[i]) {
      if (!cf.isPinHigh(i)) return i;
    }
  }
  return -1;
}

// Return the first byte in serial and remove it from there
// if nothing is available return 0
char checkSerial() {
  if (Serial.available() > 0)
    {
      return Serial.read();
    }
    return 0; // NUL
}

void sendBounds(Bounds bounds, uint8_t nodeId) {
  bus.send_packet_blocking(nodeId, &bounds, sizeof(bounds));
}

void disconnectedNodeToSerial(uint8_t found_id, uint8_t dir) {
  String s = String(nodeDisconnected);
  s += " ";
  s += found_id;
  s += ":";
  s += dir;
  toSerialWithCRC(s);
}

void setup()
{
  Serial.begin(9600);
  cf.setPinsInput();
  bus.strategy.set_pins(inputPin, outputPin);
  bus.begin();
  bus.set_receiver(receiver_function);
  // Make sure that all nodes are reset before starting
  uint8_t packet[1] = {reset};
  bus.send_packet_blocking(PJON_BROADCAST, packet, 1);
};

void loop()
{
  char serial = checkSerial();
  if (!interfaceReady)
  {
    if (serial == start) {
       runConfiguration();
    }
    else return;
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

  if (serial == quit)
  {
      interfaceReady = false;
      uint8_t packet[1] = {reset};
      bus.send_packet_blocking(PJON_BROADCAST, packet, 1);
      stackTop = 0;
      nextId = 1;
      return;
 }

 if (serial == bounds)
 { // Parse the string
  Bounds bounds;
  char input[BOUNDS_SIZE + 1];
  byte size = Serial.readBytes(input, BOUNDS_SIZE);
  // Add the final 0 to end the C string
  input[size] = 0;

  // Read each value
  char* val = strtok(input, ":");
  uint8_t nodeId = atoi(val);
  val = strtok(0, ":");
  bounds.componentType = val[0];
  val = strtok(0, ":");
  bounds.componentId = atoi(val);
  val = strtok(0, ":");
  bounds.minValue = atof(val);
  val = strtok(0, ":");
  bounds.maxValue = atof(val);
  val = strtok(0, ":");
  bounds.stepSize = atof(val);
  sendBounds(bounds, nodeId);
 }
 
  bus.receive(1500);
  // Check if a node has disconnected
  int8_t disconnectedNode = checkNodes();
  if (disconnectedNode >= 0) {
    directionToCheck[disconnectedNode] = false;
    disconnectedNodeToSerial(0, disconnectedNode);
  }

};
