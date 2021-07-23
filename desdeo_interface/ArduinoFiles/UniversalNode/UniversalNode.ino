#include <DirectionPins.h>
#include <Datatypes.h>
#include <EEPROM.h>
#include <Component.h>
#include <Potentiometer.h>
#include <RotaryEncoder.h>
#include <Button.h>
#include <CRC8.h>
#include <PJONSoftwareBitBang.h>
#include <WebUSB.h>
#define SERIAL_SIZE 30 //For receiving more complex data from serial
//#define Serial WebUSBSerial
WebUSB WebUSBSerial(1 /* http:// */, "localhost:3000");

uint8_t nt;
bool isMaster = false; // TODO check if multiple masters?
bool configured = false;

// max 2 of each, currently not even that
const uint8_t maxPots = 1;
const uint8_t maxRots = 1;
const uint8_t maxButtons = 2;

const uint8_t potPins[maxPots] = {A0}; // The pins the potentiometers are connected
const uint8_t rotPins[maxRots][2] = {{2, 3}}; // Change these to match easyeda
const uint8_t bPins[maxButtons] = {19, 20}; // Buttons

Potentiometer pots[maxPots];
RotaryEncoder rots[maxRots];
Button buttons[maxButtons];

// Communication
uint8_t id = PJON_NOT_ASSIGNED; // Temporary id, until gets assigned a unique id by the master
PJONSoftwareBitBang bus(id);
const uint8_t outputPin = 8; // master will have these pins reversed
const uint8_t inputPin = 4;
const uint8_t masterId = 254;
bool interfaceReady = false;

// Directions of known nodes, these will be checked every iterations of loop
bool directionsToCheck[4];

// For setting direction pins and configuration
DirectionPins dp = DirectionPins(7,15,14,16); // UP, RIGHT, DOWN, LEFT
//DirectionPins dp = DirectionPins(7, 5, 6, 9); // for nanos and unos

// Slave specific
bool waitingForLight = false;
bool latestReceiver = false;

// Master specific:
// more than 150 will most likely cause memory issues
const uint8_t maxNodes = 50;

// For configuration finder and dynamic ids
uint8_t nextId = 1;   // upto 253, Do not start from 0 as that is for broadcasting
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
  Serial.println(char(payload[0]));
  if (char(payload[0]) == start)
  { // Start configuration state
    dp.setPinsInput();
    waitingForLight = true;
  }
  else if (char(payload[0]) == dirToCheck)
  {
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
      dp.setPinsInput(); // Set back to input mode. In case disconnecting happens
    }
  }
  else if (char(payload[0]) == dirInstruction)
  { // New instructions
    uint8_t dir = payload[1];
    dp.setPinsInput(); // Reset pins first
    dp.setPinLow(dir);
  }
  else if (char(payload[0]) == csCompleted)
  { // CS completed, interface is ready and configured
    setDirectionPins();
    interfaceReady = true;
  }
  else
  { // Bounds struct
    BoundsData b;
    memcpy(&b, payload, sizeof(b));
    if (b.componentType == 'R')
    {
      rots[b.componentId].setBounds(b.minValue, b.maxValue, b.stepSize);
    }
    else if (b.componentType == 'P')
    {
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
  for (int i = 0; i < 4; ++i)
  {
    directionsToCheck[i] = false;
  }
  dp.setPinsInput();
  id = PJON_NOT_ASSIGNED;
  bus.set_id(id);
  //n = getNode(empty);
}

// Send initial info
void sendComponentInfo()
{
  uint8_t content[2] = {nodeInfo, nt};
  bus.send_packet_blocking(masterId, content, 2);
}

// Shared functions

void setDirectionPins()
{
  dp.setPinsInput();
  for (int i = 0; i < 4; ++i)
  {
    if (!directionsToCheck[i])
    {
      dp.setPinLow(i);
    }
  }
}

// Initialize each component
void initializeComponents()
{
  ComponentCounts c = getCounts(nt);
  for (int i = 0; i < c.potCount; i++)
  {
    Potentiometer pot = Potentiometer(potPins[i], i);
    pots[i] = pot;
    //components[i] = pot;
  }

  for (int i = 0; i < c.rotCount; i++)
  {
    RotaryEncoder rot = RotaryEncoder(rotPins[i][0], rotPins[i][1], i);
    rots[i] = rot;
    //components[potCount + i] = rot;
  }

  for (int i = 0; i < c.butCount; i++)
  {
    Button button = Button(bPins[i], i);
    buttons[i] = button;
    //components[potCount + rotCount + i] = button;
  }
}

void load()
{
  EEPROM.get(0,nt);
  if (nt == 0) return;
  configured = true;
  Serial.println("configuration loaded");
//  //Serial.flush();
}

void save()
{
  EEPROM.put(0, nt);
  Serial.println("Configuration saved");
//  //Serial.flush();
}

// Check if potentiometers have changed
void checkPots(bool blocking = false)
{
  ComponentCounts c = getCounts(nt);
  for (int i = 0; i < c.potCount; i++)
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
      else
        sendData(data, blocking);
    }
  }
};

void checkRots(bool blocking = false)
{
  ComponentCounts c = getCounts(nt);
  for (int i = 0; i < c.rotCount; i++)
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
      else
        sendData(data, blocking);
    }
  }
}

void checkButtons(bool blocking = false)
{
  ComponentCounts c = getCounts(nt);
  for (int i = 0; i < c.butCount; i++)
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
      else
        sendData(data, blocking);
    }
  }
}

// Check for directions which have a node
// Return a disconnected node id if one found
// else -1 indicating everything is fine
int8_t checkNodes()
{
  for (int i = 0; i < 4; ++i)
  {
    if (directionsToCheck[i])
    {
      if (!dp.isPinLow(i))
        return i;
    }
  }
  return -1;
}

// Return the first byte in serial and remove it from there
// if nothing is available return 0
char checkSerial()
{
  if (Serial.available() > 0)
  {
    return Serial.read();
  }
  return 0; // NUL
}

void setup()
{
  Serial.begin(9600);
  load();
  initializeComponents();
  setup_slave();
}

void setup_master()
{
    id = masterId;
    bus.strategy.set_pins(outputPin, inputPin);
    bus.set_receiver(receiver_function_master);
    bus.begin();
    bus.set_id(id);

    uint8_t packet[1] = {reset};
    bus.send_packet_blocking(PJON_BROADCAST, packet, 1);
}

void setup_slave()
{
     bus.strategy.set_pins(inputPin, outputPin);
     bus.set_receiver(receiver_function_slave);
     bus.begin();
     bus.set_id(id);

     uint8_t packet[1] = {nodeConnected};
     bus.send_packet_blocking(masterId, packet, 1); // What if no master yet
}

void loop()
{
  char serial = checkSerial();
  
  if (serial == configure)
  {
    uint8_t type = Serial.parseInt();
    //todo Vaidate type
    nt = type;
    save();
    initializeComponents();
  }

 if (!interfaceReady)
  {
    if (serial == start)
    {
      isMaster = true;
      setup_master();
      runConfiguration();
    }
  }
  
  if (!configured) {
    return;
  }
  
  bus.receive(1500);
  
  if (isMaster){
     masterLoop(serial);
  }
  else {
    slaveLoop();
  }

  if (!interfaceReady)
    return;

  // Check for changing values in components
  bus.receive(1500);
  checkPots(); //ADC
  bus.receive(1500);
  checkRots(); //todo Interupts
  checkButtons();
  
  // Check for disconnected nodes
  int8_t disconnectedNode = checkNodes();
  if (disconnectedNode >= 0)
  {
    directionsToCheck[disconnectedNode] = false;
    if (isMaster)
    {
      disconnectedNodeToSerial(id, disconnectedNode);
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
  if (!interfaceReady) return;
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
    BoundsData b;
    char input[SERIAL_SIZE + 1];
    byte size = Serial.readBytes(input, SERIAL_SIZE);
    // Add the final 0 to end the C string
    input[size] = 0;

    // Read each value
    char *val = strtok(input, ":");
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
  if (!interfaceReady)
  {  
    if (waitingForLight)
    {
      if (dp.isAnyPinLow())
      {
        latestReceiver = true;
        sendComponentInfo();
        bus.receive(2500);
      }
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
    info += payload[1];
    info += ":";
    info += (currentPos);
    info += " ";
    toSerialWithCRC(info);
  }
  if (interfaceReady)
  {
      if (char(payload[0]) == nodeConnected)
      { // A node connected
        Serial.println(nodeConnected);
        interfaceReady = false;
        runConfiguration();
      }
      else if (char(payload[0]) == nodeDisconnected)
      { // A node disconnected
        uint8_t nodeId = payload[1];
        uint8_t dir = payload[2];
        disconnectedNodeToSerial(nodeId, dir);
      }
      else
      { // Node sending data, figure out a better way
        Data data;
        memcpy(&data, payload, sizeof(data));
        String dataS = dataToString(data);
        toSerialWithCRC(dataS);
      }
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

  StackPair stack[maxNodes]; // Use this array like a stack.
  configuration(stack);

  interfaceReady = true;
  dp.setPinsInput();

  //Configuration done
  Serial.println(csCompleted);
  //Serial.flush();

  // Let the slaves know that configuration is done
  packet[0] = csCompleted;
  bus.send_packet_blocking(PJON_BROADCAST, packet, 1);
}

// Initial configuration: Assign ids dynamically to nodes,
// and find their positions
// stack: Initally this is a array where all pairs are equal to (0,0)
// The stack will be updated as the configuration continues
void configuration(StackPair stack[maxNodes])
{
  uint8_t dir = stack[stackTop].dir;
  updateCurrentPos(stack);
  if (dir == 4) // Invalid direction
  {
    if (stackTop == 0)
    { // Master checked all its directions
      return;
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
    dp.setPinLow(dir);
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
    if (stackTop == 0)
    {
      directionsToCheck[dir] = true;
    }
    else
    {
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
void updateCurrentPos(StackPair stack[maxNodes])
{
  currentPos = ""; // Reset
  for (int i = 0; i <= stackTop; i++)
  {
    currentPos += (stack[i].dir);
  }
}

void sendBounds(BoundsData b, uint8_t nodeId)
{
  bus.send_packet_blocking(nodeId, &b, sizeof(b));
}

void disconnectedNodeToSerial(uint8_t found_id, uint8_t dir)
{
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
  dataS += String(data.value, 5);
  return dataS;
}

void toSerialWithCRC(String s)
{
  uint8_t data[s.length() + 1];
  s.getBytes(data, s.length() + 1);
  uint8_t crc8 = crc.getCRC8(data, s.length());
  Serial.print(s);
  Serial.print(" ");
  Serial.println(crc8);
  //Serial.flush();
}
