#include <Datatypes.h>
#include <EEPROM.h>
#include <ConfiguationFinder.h>
#include <Component.h>
#include <Potentiometer.h>
#include <RotaryEncoder.h>
#include <Button.h>
#include <CRC8.h>
#include <PJONSoftwareBitBang.h>
#define SERIAL_SIZE 30

struct MyEeprom 
{
  bool configured;
  bool isMaster;
  uint8_t type;
};

typedef struct
{
  uint8_t nodeId;
  uint8_t dir = 0;
} stackPair;

// This is the data structure that is send to the master
struct Data
{
  uint8_t nodeId;
  double value;
  uint8_t id;
  char type;
};

//struct Bounds 
//{
//  char componentType;
//  uint8_t componentId;
//  double minValue;
//  double maxValue;
//  double stepSize;
//};

//const char configure = 'F';
//const char nodeInfo = 'N';
//const char nodeConnected = 'C'; 
//const char nodeDisconnected = 'D';
//const char componentValue = 'V';
//const char start = 'S';
//const char quit = 'Q';
//const char reset = 'E';
//const char idPacket = 'I';
//const char dirInstruction = 'N';
//const char csCompleted = 'O';
//const char dirToCheck = 'D';
//const char bounds = 'B';


node n; // TODO should have the isMaster in it
bool isMaster = false; // TODO check if multiple masters?
bool configured = false;

// max 3 of each
const uint8_t maxComponents = 3;
const uint8_t potPins[maxComponents] = {A0, A1, A2};  // The pins the potentiometers are connected
const uint8_t rotPins[maxComponents][2] = {{2,3}, {4,5}, {6,7}};
const uint8_t bPins[maxComponents] = {10,11,12};   // Buttons

Potentiometer pots[maxComponents];
RotaryEncoder rots[maxComponents];
Button buttons[maxComponents];
//const uint8_t compCount =  potCount + rotCount + buttonCount;
//Component components[compCount];

// Communication
uint8_t id = PJON_NOT_ASSIGNED; // Temporary id, until gets assigned a unique id by the master
PJONSoftwareBitBang bus(id);
const uint8_t outputPin = 8;
const uint8_t inputPin = 4;
const uint8_t masterId = 254;
bool interfaceReady = false;

// Direction where a node is
bool directionsToCheck[4];

// For setting direction pins and configuration
//ConfigurationFinder cf = ConfigurationFinder(7,15,14,16); // UP, RIGHT, DOWN, LEFT
ConfigurationFinder cf = ConfigurationFinder(7,5,6,9);  // for nanos and unos

// Slave specific
bool waitingForLight = false;
bool latestReceiver = false;

// Master specific:
// more than 150 will cause memory issues
const uint8_t maxNodes = 128;

// For configuration finder and dynamic ids
uint8_t nextId = 1; // upto 253, Do not start from 0 as that is for broadcasting
uint8_t stackTop = 0; // master at bottom
// TODO currentPos should be byte array
String currentPos = ""; // Keep track of the current position we're in the configuration stage

// For cyclic redundancy check
const uint8_t crcKey = 7; // This key has to be same on both sides
CRC8 crc = CRC8(crcKey);


// Slave functions:

// This function is called whenever a packet is received
void receiver_function_slave(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info)
{
  if (char(payload[0]) == start)
  { // Start configuration state
        cf.setPinsInput();
        waitingForLight = true;
  }
  else if (char(payload[0]) == dirToCheck) {
    uint8_t dir = payload[1];
    directionsToCheck[dir] = true;
  }
  else if (char(payload[0]) == reset)
  { // Reset everything
    setSelfToInitialMode();
  }
  else if (char(payload[0]) == idPacket && id == PJON_NOT_ASSIGNED)
  { // ID
    if (latestReceiver)
    {
      latestReceiver = false;
      id = payload[1];
      bus.set_id(id);
      waitingForLight = false;
      cf.setPinsInput(); // Set back to input mode. In case disconnecting happens
    }
  }
  else if (char(payload[0]) == dirInstruction)
  { // New instructions
    uint8_t dir = payload[1];
    cf.setPinsInput(); // Reset pins first
    cf.setPinHigh(dir);
  }
  else if (char(payload[0]) == csCompleted) 
  { // CS completed, interface is ready and configured
    setDirectionPins();
    interfaceReady = true;
  }
  else
  { // Bounds struct
    bounds_data b;
    memcpy(&b, payload, sizeof(b));
    if (b.componentType == 'R') {
      rots[b.componentId].setBounds(b.minValue, b.maxValue, b.stepSize);
    }
    else if (b.componentType == 'P') {
      pots[b.componentId].setBounds(b.minValue, b.maxValue);
    }
  }
}

// Send the Data struct to master
bool sendData(Data data, bool blocking = false)
{
  data.nodeId = id;
  uint16_t packet = blocking ? bus.send_packet_blocking(masterId, &data, sizeof(data)) : bus.send_packet(masterId, &data, sizeof(data));
  return (packet != PJON_ACK);
}


// should propably reset component bounds as well
void setSelfToInitialMode() 
{
    interfaceReady = false;
    waitingForLight = false;
    latestReceiver = false;
    for (int i = 0; i < 4; ++i) {
       directionsToCheck[i] = false;
    }
    cf.setPinsInput();
    id = PJON_NOT_ASSIGNED;
    bus.set_id(id);
    //n = getNode(empty);
}

// Send initial info
void sendComponentInfo()
{
  uint8_t content[4] = {nodeInfo, n.potCount, n.rotCount, n.butCount};
  bus.send_packet_blocking(masterId, content, 4);
}


// Shared functions

void setDirectionPins() {
  cf.setPinsInput();
  for (int i = 0; i < 4; ++i) {
    if (!directionsToCheck[i]) {
      cf.setPinHigh(i);
    }
  }
}

// Initialize each component
void initializeComponents() {
  for (int i = 0; i < n.potCount; i++)
  {
    Potentiometer pot = Potentiometer(potPins[i], i);
    pots[i] = pot;
    //components[i] = pot;
  }

  for (int i = 0; i < n.rotCount; i++)
  {
    RotaryEncoder rot = RotaryEncoder(rotPins[i][0], rotPins[i][1], i);
    rots[i] = rot;
    //components[potCount + i] = rot;
  }

  for (int i = 0; i < n.butCount; i++)
  {
    Button button = Button(bPins[i], i);
    buttons[i] = button;
    //components[potCount + rotCount + i] = button;
  }
}

// Load values myEeprom from the EEMPROM and set the values
void loadValues() 
{
 MyEeprom epr;
 EEPROM.get(0, epr);
 if (!epr.configured)
 {
  Serial.println("Configure this node with a command of type 'F isMaster:typeNumber'");
  return;
 }
 configured = true;
 isMaster = epr.isMaster;
  n = getNode(epr.type);

 Serial.println("configuration loaded");
  initializeComponents();
}


// Save node configuration to EEPROM
void saveValues(bool isMast, uint8_t type)
{
  MyEeprom epr;
  epr.configured = true;
  epr.isMaster = isMast;
  // TODO validate type
  epr.type = type;
  EEPROM.put(0, epr);
  Serial.println("Configuration saved");
  loadValues();
}

// Check if potentiometers have changed
void checkPots(bool blocking = false)
{
  for (int i = 0; i < n.potCount; i++)
  {
    Potentiometer pot = pots[i];
    double value = pot.getValue();
    pots[i] = pot;
    if (pot.hasChanged() || blocking)
    {
      Data data;
      data.value = value;
      data.id = pot.getId();
      data.type = pot.getType();
      if (isMaster) 
      {
        data.nodeId = masterId;
        String dataS = dataToString(data);
        toSerialWithCRC(dataS);
      }
      else sendData(data, blocking);
    }
  }
};

void checkRots(bool blocking = false)
{
  for (int i = 0; i < n.rotCount; i++)
  {
    RotaryEncoder rot = rots[i];
    double value = rot.getValue();
    rots[i] = rot;
    if (rot.hasChanged() || blocking)
    {
      Data data;
      data.value = value;
      data.id = rot.getId();
      data.type = rot.getType();
      if (isMaster) 
      {
        data.nodeId = masterId;
        String dataS = dataToString(data);
        toSerialWithCRC(dataS);
      }
      else sendData(data, blocking);
    }
  }
}

void checkButtons(bool blocking = false)
{
  for (int i = 0; i < n.butCount; i++)
  {
    Button button = buttons[i];
    double value = button.getValue();
    buttons[i] = button;
    if (button.hasChanged() || blocking)
    {
      Data data;
      data.value = value;
      data.id = button.getId();
      data.type = button.getType();
      if (isMaster) 
      {
        data.nodeId = masterId;
        String dataS = dataToString(data);
        toSerialWithCRC(dataS);
      }
      else sendData(data, blocking);
    }
  }
}

// Check for directions which have a node
// Return a disconnected node id if one found
// else -1 indicating everything is fine
int8_t checkNodes() {
  for (int i = 0; i < 4; ++i) {
    if (directionsToCheck[i]) {
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

void setup()
{
  Serial.begin(9600);
  loadValues(); // This will also initialize components

  bus.strategy.set_pins(inputPin, outputPin);
  bus.begin();
  
  if (isMaster) 
  {
    id = masterId;
    bus.set_receiver(receiver_function_master);
  }
  else 
  {
    bus.set_receiver(receiver_function_slave);
  }
  
  bus.set_id(id);

  if (isMaster) 
  { // Send a reset command to nodes
      uint8_t packet[1] = {reset};
      bus.send_packet_blocking(PJON_BROADCAST, packet, 1);
  }
  else
  { // Notify master of connecting
    uint8_t packet[1] = {nodeConnected};
    bus.send_packet_blocking(masterId, packet, 1); 
  }

}

void loop()
{
  char serial = checkSerial();
  if (serial == configure)
  {
      char input[SERIAL_SIZE + 1];
      byte size = Serial.readBytes(input, SERIAL_SIZE);
      input[size] = 0;
      char* val = strtok(input, ":");
      bool isMaster = atoi(val);
      val = strtok(0, ":");
      uint8_t type = atoi(val);
      saveValues(isMaster, type);
  }
  if (!configured) return;
  bus.receive(1500); // It is not guaranteed that slaves will receive the 'Q' command from the master
  if (isMaster) masterLoop(serial);
  else slaveLoop();
  if (!interfaceReady) return;

  // Check for changing values in components
  bus.receive(1500);
  checkPots(); //ADC
  bus.receive(1500);
  checkRots(); //Interupts
  checkButtons();

  // Check for disconnected nodes
  int8_t disconnectedNode = checkNodes();
  if (disconnectedNode >= 0) {
    directionsToCheck[disconnectedNode] = false;
    if (isMaster) 
    {
      disconnectedNodeToSerial(0, disconnectedNode);
    }
    else 
    {
      uint8_t content[3] = {nodeDisconnected, id, disconnectedNode};
      bus.send_packet_blocking(masterId, content, 3);
    }
  } 
};

void masterLoop(char serial)
{
  if (!interfaceReady)
  {
    if (serial == start) {
       runConfiguration();
    }
    else return;
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
  bounds_data b;
  char input[SERIAL_SIZE + 1];
  byte size = Serial.readBytes(input, SERIAL_SIZE);
  // Add the final 0 to end the C string
  input[size] = 0;

  // Read each value
  char* val = strtok(input, ":");
  uint8_t nodeId = atoi(val);
  val = strtok(0, ":");
  b.componentType = val[0];
  val = strtok(0, ":");
  b.componentId = atoi(val);
  val = strtok(0, ":");
  b.minValue = atof(val);
  val = strtok(0, ":");
  b.maxValue = atof(val);
  val = strtok(0, ":");
  b.stepSize = atof(val);
  sendBounds(b, nodeId);
 }
}

void slaveLoop()
{
  if (interfaceReady)  return;
  
  if (waitingForLight)
  {
    if (cf.isAnyPinHigh())
    {
      latestReceiver = true;
      sendComponentInfo();
      bus.receive(2500);
    }
  }
}

// Master functions:
void receiver_function_master(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info)
{
  if (char(payload[0]) == nodeInfo) 
  { // OK: Node received data in configuration state and send component info
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

// Configuration checking
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
    uint8_t content2[2] = {idPacket, nextId};
     bus.send_packet_blocking(PJON_BROADCAST, content2, 2); // Send the id to the slave
      if (stackTop == 0) {
        directionsToCheck[dir] = true;
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

void sendBounds(bounds_data b, uint8_t nodeId) {
  bus.send_packet_blocking(nodeId, &b, sizeof(b));
}

void disconnectedNodeToSerial(uint8_t found_id, uint8_t dir) {
  String s = String(nodeDisconnected);
  s += " ";
  s += found_id;
  s += ":";
  s += dir;
  toSerialWithCRC(s);
}

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

void toSerialWithCRC(String s) {
   uint8_t data[s.length() + 1];
   s.getBytes(data, s.length() + 1);
   uint8_t crc8 = crc.getCRC8(data, s.length());
   Serial.print(s); Serial.print(" ");
   Serial.println(crc8);
}
