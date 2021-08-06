#include <Led.h>
#include <ADS1X15.h>
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

#define ADS_ADDRESS 0x48
#define SERIAL_SIZE 30 //Buffer for receiving data from serial. Shouldn't exceed this limit
#define webusbSerial WebUSBSerial

//Could be simplified: use less memory
// Which serial is being used
int8_t whichSerial = 0; // 0 undefined, -1 reg serial, 1 webusb

// Instansiate webusb
WebUSB WebUSBSerial(1 /* http:// */, "127.0.0.1:5500/desdeo_interface/web/index.html");

// Instansiate ads
ADS1115 ADS(ADS_ADDRESS); //Only call begin method if pot is connected

NodeType nt;           // This nodes nodetype. One can set a nodetype from serial with command 'F {nodetype}'
bool hasType = false;  // Does the node have a valid (not empty) nodetype
bool isMaster = false; // Is the node the master. The node which receives the start command from Serial will be declared as the Master

// All of these are currently limited by the schematic/pin count. The code supports having multiple of each
const uint8_t maxPots = 1;    // The maximum count of potentiometers a node can have.
const uint8_t maxRots = 1;    // The maximum count of rotary encoders a node can have.
const uint8_t maxButtons = 1; // The maximum count of buttons a node can have. Note that a rotary encoder acts as a button and is not counted here.

const uint8_t potPins[maxPots] = {0};           // The ADS pins the potentiometers are connected to
const uint8_t rotPins[maxRots][2] = {{A2, A3}}; // Rotary encoder pins
const uint8_t rotBPins[maxRots] = {A0};         // Rotary encoder button pin
const uint8_t bPins[maxButtons] = {A1};         // Button pins

// Instanstiating empty components. These will be overwritten in setup
Potentiometer pots[maxPots];
RotaryEncoder rots[maxRots];
Button buttons[maxButtons + maxRots];

// Instansiate the rgb led component.
Led led = Led(9, 6, 5);

// Declare variables used in PJON
uint8_t id = PJON_NOT_ASSIGNED; // Temporary id, until gets assigned a unique id by the master
PJONSoftwareBitBang bus(id);    // Instansiate the PJON bus
const uint8_t outputPin = 8;    // For master this is 4
const uint8_t inputPin = 4;     // For master this is 8
const uint8_t masterId = 254;   // Master id. which always know by every node and doesn't ever change

bool interfaceReady = false; // Is the initial configuration done

bool directionsToCheck[4]; // Directions to be checked after configuration state. Used for disconnecting nodes.

const uint8_t dirs[4] = {7, 15, 14, 16};                              // Digital pins: UP, RIGHT, DOWN, LEFT
DirectionPins dp = DirectionPins(dirs[0], dirs[1], dirs[2], dirs[3]); // Instansiate direction pins object

// Node specific

// used in configuration state
bool waitingForLight = false; // Is the node waiting for signal
bool latestReceiver = false;  // Is the node the latest receiver

// Master specific:

// more than 150 will most likely cause memory issues
const uint8_t maxNodes = 50; // What is the max count of nodes in the configuration state stack

// used in configuration state
uint8_t nextId = 1;   // upto 253, Do not start from 0 as that is reserved for PJON broadcast
uint8_t stackTop = 0; // Initally master is at top, we use id 0 here for master for simplicity.
// TODO currentPos should be byte array
String currentPos = ""; // The position the configuration state is checking at that moment. Updated every loop

// For cyclic redundancy check
const uint8_t crcKey = 7; // This key has to be same on both sides
CRC8 crc = CRC8(crcKey);

// Slave functions:

// This function is called whenever a packet is received
// Each packet expect a bounds packet has an id, located at payload[0] which is used to determine the action.
void receiver_function_slave(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info)
{
  char command = char(payload[0]);

  if (command == start)
  { // Start configuration state
    handleStartPacket();
  }
  else if (command == dirToCheck)
  {
    uint8_t dir = payload[1];      // This direction should be checked after configuration state
    directionsToCheck[dir] = true; // save that information
  }
  else if (command == toInitial)
  {
    setSelfToInitialMode();
  }
  else if (command == idPacket && id == PJON_NOT_ASSIGNED)
  {
    if (latestReceiver)
    {
      handleIdPacket(payload[1]);
    }
  }
  else if (command == dirInstruction)
  { // New direction instructions
    handleDirInstructionPacket(payload[1]);
  }
  else if (command == csCompleted)
  { // CS completed, interface is ready and configured
    handleCSCompletedPacket();
  }
  else
  { // Bounds struct
    handleBoundsPacket(payload);
  }
}

void handleStartPacket()
{
  led.setColor(inConfigurationStateColor);
  dp.setPinsInput();         // Set all pins to input
  waitingForLight = true;    // Node should expect a signal.
}

void handleIdPacket(uint8_t newId)
{
  latestReceiver = false;
  id = newId;
  bus.set_id(id);
  waitingForLight = false;
  dp.setPinsInput(); // Set back to input mode. In case disconnecting happens
}

void handleDirInstructionPacket(uint8_t dir)
{
  dp.setPinsInput(); // All pins to input first
  dp.setPinLow(dir); // Set given direction to low
}

void handleCSCompletedPacket()
{
  led.setColor(configuredColor); // green
  setDirectionPins();
  interfaceReady = true;
}

void handleBoundsPacket(uint8_t *payload)
{
  BoundsData b;
  memcpy(&b, payload, sizeof(b));
  setBounds(b);
}

// Send the Data struct to master
bool sendData(Data data)
{
  data.nodeId = id;
  uint16_t packet = bus.send_packet_blocking(masterId, &data, sizeof(data));
  return (packet != PJON_ACK);
}

// should propably reset component bounds as well
void setSelfToInitialMode()
{
  led.setColor(initialStateColor); // Orange
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

void setBounds(BoundsData b)
{
  if (b.componentType == 'R')
  {
    rots[b.componentId].setBounds(b.minValue, b.maxValue, b.stepSize);
  }
  else if (b.componentType == 'P')
  {
    pots[b.componentId].setBounds(b.minValue, b.maxValue);
  }
}

void setADS()
{
  ADS.begin();
  ADS.setGain(1);     // ±4.096V
  ADS.setDataRate(7); // Fastest
}

// Initialize each component
void initializeComponents()
{
  ComponentCounts c = getCounts(nt);

  if (c.potCount > 0)
  {
    setADS();
  }

  for (int i = 0; i < c.potCount; i++)
  {
    Potentiometer pot = Potentiometer(potPins[i], i);
    pot.activate();
    pots[i] = pot;
  }

  for (int i = 0; i < c.rotCount; i++)
  {
    uint8_t pin1 = rotPins[i][0];
    uint8_t pin2 = rotPins[i][1];
    RotaryEncoder rot = RotaryEncoder(pin1, pin2, i);
    rot.activate();
    rots[i] = rot;

    Button button = Button(rotBPins[i], i);
    button.activate();
    buttons[i] = button;
  }

  for (int i = 0; i < c.butCount; i++)
  {
    Button button = Button(bPins[i], c.rotCount + i);
    button.activate();
    buttons[c.rotCount + i] = button;
  }
}

void load()
{
  EEPROM.get(0, nt);
  if (nt == 0)
    return;
  hasType = true;
  if (whichSerial == -1)
    Serial.println("configuration loaded");
}

void save()
{
  EEPROM.put(0, nt);
  if (whichSerial == -1)
    Serial.println("Configuration saved");
}

// Check if potentiometers have changed
void checkPots()
{
  ComponentCounts c = getCounts(nt);
  for (int i = 0; i < c.potCount; i++)
  {
    Potentiometer pot = pots[i];
    double val = pot.getValue(ADS);
    if (pot.hasChanged())
    {
      led.setColor(sendingDataColor);
      Data data;
      data.value = val;
      data.id = pot.getId();
      data.type = pot.getType();
      if (isMaster)
      {
        data.nodeId = masterId;
        String dataS = dataToString(data);
        toSerialWithCRC(dataS);
      }
      else
        sendData(data);
    }
    led.setColor(configuredColor); // green
    pots[i] = pot;
  }
};

void checkRots()
{
  ComponentCounts c = getCounts(nt);
  for (int i = 0; i < c.rotCount; i++)
  {
    RotaryEncoder rot = rots[i];
    double val = rot.getValue();
    if (rot.hasChanged())
    {
      led.setColor(sendingDataColor);
      Data data;
      data.value = val;
      data.id = rot.getId();
      data.type = rot.getType();
      rots[i] = rot;
      if (isMaster)
      {
        data.nodeId = masterId;
        String dataS = dataToString(data);
        toSerialWithCRC(dataS);
      }
      else
        sendData(data);
    }
    led.setColor(configuredColor); // green
    rots[i] = rot;
  }
}

void checkButtons()
{
  ComponentCounts c = getCounts(nt);
  for (int i = 0; i < c.butCount + c.rotCount; i++)
  {
    Button button = buttons[i];
    double value = button.getValue();
    if (button.hasChanged())
    {
      led.setColor(sendingDataColor);
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
        sendData(data);
    }
    led.setColor(configuredColor); // green
    buttons[i] = button;
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
  if (whichSerial == 0)
  { // checkboth
    if (Serial.available() > 0)
    {
      whichSerial = -1;
      return Serial.read();
    }
    if (webusbSerial.available() > 0)
    {
      whichSerial = 1;
      return webusbSerial.read();
    }
  }
  else if (whichSerial == 1 && webusbSerial.available() > 0)
    return webusbSerial.read();
  else if (whichSerial == -1 && Serial.available() > 0)
    return Serial.read();

  return 0; // NUL
}

void setup()
{
  led.setColor(initialStateColor);
  Serial.begin(9600);
  webusbSerial.begin(9600);
  load();
  initializeComponents();
  if (!isMaster)
    setup_slave();
}

void setup_master()
{
  id = masterId;
  bus.strategy.set_pins(outputPin, inputPin);
  bus.set_receiver(receiver_function_master);
  bus.begin();
  bus.set_id(id);

  uint8_t packet[1] = {toInitial};
  bus.send_packet_blocking(PJON_BROADCAST, packet, 1);
}

void setup_slave()
{
  bus.strategy.set_pins(inputPin, outputPin);
  bus.set_receiver(receiver_function_slave);
  bus.begin();
  bus.set_id(id);

  uint8_t packet[1] = {nodeConnected};
  bus.send_packet_blocking(masterId, packet, 1);
}

void loop()
{
  char serial = checkSerial();

  if (serial == configure)
  {
    uint8_t type = (whichSerial == -1) ? Serial.parseInt() : webusbSerial.parseInt();
    //todo Validate type
    nt = type;
    save();
  }

  if (!interfaceReady)
  {
    if (serial == start)
    {
      led.setColor(inConfigurationStateColor);
      isMaster = true;
      setup_master();
      runConfiguration();
    }
  }

  if (!hasType)
  {
    return;
  }

  bus.receive(350);

  if (isMaster)
  {
    masterLoop(serial);
  }
  else
  {
    slaveLoop();
  }
  if (!interfaceReady)
    return;

  // Check for changing values in components
  bus.receive(350);
  checkPots();
  checkRots();
  checkButtons();

  bus.receive(150);

  handleDisconnectedNodes();

  bus.receive(150);
};

void handleDisconnectedNodes()
{
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
}

void masterLoop(char serial)
{
  if (!interfaceReady)
    return;
  if (serial == quit)
  {
    handleQuitSerial();
    return;
  }

  if (serial == bounds)
  {
    handleBoundsSerial();
  }
}

void handleQuitSerial()
{
  led.setColor(initialStateColor);
  interfaceReady = false;
  uint8_t packet[1] = {toInitial};
  bus.send_packet_blocking(PJON_BROADCAST, packet, 1);
  stackTop = 0;
  nextId = 1;
  whichSerial = 0;
}

void handleBoundsSerial()
{
  BoundsData b;
  char input[SERIAL_SIZE + 1];
  byte size = (whichSerial == -1) ? Serial.readBytes(input, SERIAL_SIZE) : webusbSerial.readBytes(input, SERIAL_SIZE);
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

  if (nodeId == masterId || nodeId == 0)
    setBounds(b);
  else
    sendBounds(b, nodeId);
}

void slaveLoop()
{
  checkForSignal();
}

void checkForSignal()
{
  if (interfaceReady)
    return;

  if (!waitingForLight)
    return;

  if (dp.isAnyPinLow())
  {
    latestReceiver = true;
    sendComponentInfo();
    bus.receive(2500);
  }
}

// Master functions:

void receiver_function_master(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info)
{
  if (char(payload[0]) == nodeInfo)
  { // OK: Node received data in configuration state and send component info
    sendNodeInfo(nextId, int(payload[1]), currentPos);
  }

  if (interfaceReady)
  {
    if (char(payload[0]) == nodeConnected)
    { // A node connected
      handleNodeConnection();
    }
    else if (char(payload[0]) == nodeDisconnected)
    { // A node disconnected
      handleNodeDisconnection(payload[1], payload[2]);
    }
    else
    { // Node sending data, figure out a better way
      handleDataPacket(payload);
    }
  }
};

void handleNodeConnection()
{
  Serial.println(nodeConnected);
  webusbSerial.println(nodeConnected);
  webusbSerial.flush();
}

void handleNodeDisconnection(uint8_t nid, uint8_t dir)
{
  disconnectedNodeToSerial(nid, dir);
}

void handleDataPacket(uint8_t *payload)
{
  Data data;
  memcpy(&data, payload, sizeof(data));
  String dataS = dataToString(data);
  toSerialWithCRC(dataS);
}

// TODO get rid of string its slow
void sendNodeInfo(uint8_t id, uint8_t nt, String pos)
{
  String info = String(nodeInfo);
  info += " ";
  info += id;
  info += ":";
  info += nt;
  info += ":";
  info += pos;
  info += " ";
  toSerialWithCRC(info);
}

// Configuration checking
void runConfiguration()
{
  // send own info
  sendNodeInfo(masterId, nt, "-1");

  // Make sure base values are correct, incase running configuration again
  uint8_t packet[1] = {toInitial};
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
  if (whichSerial == 1)
  {
    webusbSerial.println(csCompleted);
    webusbSerial.flush();
  }
  else if (whichSerial == -1)
    Serial.println(csCompleted);

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
  if (whichSerial == 1)
  {
    webusbSerial.print(s);
    webusbSerial.print(" ");
    webusbSerial.println(crc8);
    webusbSerial.flush();
  }
  else if (whichSerial == -1)
  {
    Serial.print(s);
    Serial.print(" ");
    Serial.println(crc8);
  }
}
