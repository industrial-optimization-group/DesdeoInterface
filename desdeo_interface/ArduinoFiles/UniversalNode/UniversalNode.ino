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
bool oneWire = false; // Optionally use only one wire (in D4) for pjon communication

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
const uint8_t maxNodes = 100; // What is the max count of nodes in the configuration state stack. This not same as max nodes in total

// used in configuration state
uint8_t nextId = 1;   // upto 253, Do not start from 0 as that is reserved for PJON broadcast
uint8_t stackTop = 0; // Initally master is at top, we use id 0 here for master for simplicity.
// TODO currentPos should be an array
String currentPos = ""; // The position the configuration state is checking at that moment. Updated every loop

// For cyclic redundancy check
const uint8_t crcKey = 7; // This key has to be same on both sides
CRC8 crc = CRC8(crcKey);


// Node functions:

/*
 * Function: receiver_function_node
 * --------------------
 * Is called whenever a node receives a packet.
 * 
 * payload: packet
 * length: packet length
 * packet_info: A pjon packet info that contains information on packet. Not used
 */
void receiver_function_node(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info)
{
  char command = char(payload[0]); // packet id

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


/*
 * Function: handleStartPacket 
 * --------------------
 * Handles a start packet by setting direction pins to input and sets a flag
 */
void handleStartPacket()
{
  led.setColor(inConfigurationStateColor);
  dp.setPinsInput();         // Set all pins to input
  waitingForLight = true;    // Node should expect a signal.
}

/*
 * Function: handleIdPacket 
 * --------------------
 * Handles an id packet by setting the given id to self and setting flags 
 * and directions pins 
 * 
 * newId: The new id for the node
 */
void handleIdPacket(uint8_t newId)
{
  latestReceiver = false;
  id = newId;
  bus.set_id(id);
  waitingForLight = false;
  dp.setPinsInput(); // Set back to input mode. In case disconnecting happens
}

/*
 * Function: handleDirInstructionPacket
 * --------------------
 * Handles direction instruction packet by setting the given direction pin to low
 * 
 * dir: given direction
 */
void handleDirInstructionPacket(uint8_t dir)
{
  dp.setPinsInput(); // All pins to input first
  dp.setPinLow(dir); // Set given direction to low
}

/*
 * Function: handleCSCompletedPacket
 * --------------------
 * Handles configuration completed packet by setting directions pins to correct state
 */
void handleCSCompletedPacket()
{
  led.setColor(configuredColor); // green
  setDirectionPins();
  interfaceReady = true;
}

/*
 * Function: handleBoundsPacket
 * --------------------
 * Handles a bounds packet by setting corresponding components bounds to given bounds
 * 
 * payload: the packet that has been sent.
 */
void handleBoundsPacket(uint8_t *payload)
{
  BoundsData b; // Init bounds
  memcpy(&b, payload, sizeof(b)); // Load bounds
  setBounds(b);
}

/*
 * Function: sendData
 * --------------------
 * Send a Data type to master
 * 
 * data: The data to be send.
 * 
 * Returns: was the packet send successfully
 */
bool sendData(Data data)
{
  data.nodeId = id;
  uint16_t packet = bus.send_packet_blocking(masterId, &data, sizeof(data));
  return (packet != PJON_ACK);
}

/*
 * Function: setSelfToInitialMode
 * --------------------
 * Sets the nodes global variables back to inital state
 */
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

/*
 * Function: sendComponentInfo
 * --------------------
 * Sends the node type to master in the configuration state as an confirmation packet
 */
void sendComponentInfo()
{
  uint8_t content[2] = {nodeInfo, nt};
  bus.send_packet_blocking(masterId, content, 2);
}

// Shared functions: used by both master and nodes

/*
 * Function: setDirectionPins
 * --------------------
 * Sets the direction pins low where no nodes were found
 * Other directions will be input.
 * Used to check for disconnecting nodes.
 */
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

/*
 * Function: setBounds
 * --------------------
 * Sets the direction pins low where no nodes were found
 * Other directions will be input.
 * Used to check for disconnecting nodes.
 * 
 * b: The bounds
 */
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

/*
 * Function: setADS
 * --------------------
 * Initializes the ADS module
 * Only called if the node has a potentiometer.
 * see https://github.com/RobTillaart/ADS1X15 for more details
 */
void setADS()
{
  ADS.begin();
  ADS.setGain(0);     // ±6.096V
  ADS.setDataRate(4); // 4 Default, 0 slowest, 7 fastest
}

/*
 * Function: initializeComponents
 * --------------------
 * Initializes all components.
 */
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
    pots[i] = pot;
  }

  for (int i = 0; i < c.rotCount; i++)
  {
    uint8_t pin1 = rotPins[i][0];
    uint8_t pin2 = rotPins[i][1];
    RotaryEncoder rot = RotaryEncoder(pin1, pin2, i);
    rots[i] = rot;

    Button button = Button(rotBPins[i], i);
    buttons[i] = button;
  }

  for (int i = 0; i < c.butCount; i++)
  {
    Button button = Button(bPins[i], c.rotCount + i);
    buttons[c.rotCount + i] = button;
  }
}

/*
 * Function: load
 * --------------------
 * loads the nodetype from EEPROM.
 */
void load()
{
  EEPROM.get(0, nt);
  if (nt == 0)
    return;
  hasType = true;
  if (whichSerial == -1)
    Serial.println("configuration loaded");
}

/*
 * Function: save
 * --------------------
 * saves the nodetype to EEPROM.
 */
void save()
{
  EEPROM.put(0, nt);
  if (whichSerial == -1)
    Serial.println("Configuration saved");
}

/*
 * Function: checkPots
 * --------------------
 * Checks every potentiometer that is connected.
 * If changes in values then they are sent to master or to serial
 * depending on whether the current node is master or not
 */
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
    led.setColor(configuredColor);
    pots[i] = pot;
  }
};

/*
 * Function: checkRots
 * --------------------
 * Checks every rotary encoder that is connected.
 * If changes in values then they are sent to master or to serial
 * depending on whether the current node is master or not
 */
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

/*
 * Function: checkButtons
 * --------------------
 * Checks every button that is connected including rotary encoder switches.
 * If changes in values then they are sent to master or to serial
 * depending on whether the current node is master or not
 */
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

/*
 * Function: checkNodes
 * --------------------
 * Checks if a node has disconnected by reading direction pin values.
 * Only called when configuration is completed
 * 
 * Returns: The direction of a disconnected node
 *  or -1 if no disconnected nodes.
 */
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

/*
 * Function: checkSerial
 * --------------------
 * Reads a byte from serial if available. Is also responsible of checking
 * which serial is in use: The node will use that serial which first was available
 * until a Quit packet is received.  
 * 
 * Returns: The first character from serial.
 */
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

/*
 * Function: setup
 * --------------------
 * Initializes components, serial and loads the nodetype from EEPROM.
 * Is called for both master and nodes
 */
void setup()
{
  led.setColor(initialStateColor);
  Serial.begin(9600);
  webusbSerial.begin(9600);
  load();
  initializeComponents();
  if (!isMaster) // useless check?
    setup_node();
}

/*
 * Function: setup_master
 * --------------------
 * Setups master. Mainly resposible of setting the id and pjon bus.
 */
void setup_master()
{
  id = masterId;
  oneWire ? bus.strategy.set_pin(inputPin) : 
            bus.strategy.set_pins(outputPin, inputPin);
            
  bus.set_receiver(receiver_function_master);
  bus.begin();
  bus.set_id(id);

  uint8_t packet[1] = {toInitial};
  bus.send_packet_blocking(PJON_BROADCAST, packet, 1);
}

/*
 * Function: setup_node
 * --------------------
 * Setups a node. Mainly resposible of setting the id and pjon bus.
 */
void setup_node()
{
  oneWire ? bus.strategy.set_pin(inputPin) :
            bus.strategy.set_pins(inputPin, outputPin);
 
  bus.set_receiver(receiver_function_node);
  bus.begin();
  bus.set_id(id);

  uint8_t packet[1] = {nodeConnected};
  bus.send_packet_blocking(masterId, packet, 1);
}

/*
 * Function: loop
 * --------------------
 * The main loop of the program. Is called forever and for both master and nodes.
 * Everything is handled here as interrupts caused some issues and pjon doesn't support interrupts.
 */
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
    nodeLoop();
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

/*
 * Function: handleDisconnectedNodes
 * --------------------
 * Sends information of the disconnected to master or Serial depending on wether or not 
 * the node is master or not. The packet contains this nodes id and the direction of the disconnected node.
 * These values together can be used to determine the id of the disconnected node.
 */
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

/*
 * Function: masterLoop
 * --------------------
 * Main loop for only the master.
 * Currently used for initial configuration and serial commands
 * 
 * serial: the char read from serial in main loop.
 */
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

/*
 * Function: handleQuitSerial
 * --------------------
 * Handles a quit packet from serial by setting self to inital mode and sending a reset packet to all nodes.
 */
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

/*
 * Function: handleBoundsSerial
 * --------------------
 * Handles a bounds packet from serial by parsing it and then sending the bounds to the corresponding node.
 */
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

/*
 * Function: nodeLoop
 * --------------------
 * A nodes main loop. 
 */
void nodeLoop()
{
  checkForSignal();
}

/*
 * Function: checkForSignal
 * --------------------
 * Checks for pins being pulled down in the configuration state. Not called by master.
 * If a pin is pulled down and is waiting for it then sends information to master
 */
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

/*
 * Function: receiver_function_master
 * --------------------
 * Called whenever a the master receives a packet from a node.
 */
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

/*
 * Function: handleNodeConnection
 * --------------------
 * Handles a new node connection packet from a node after configuration state by passing the information to serial
 */
void handleNodeConnection()
{
  Serial.println(nodeConnected);
  webusbSerial.println(nodeConnected);
  webusbSerial.flush();
}

/*
 * Function: handleNodeDisconnection
 * --------------------
 * Handles a node disconnection packet from a node after configuration state by passing the information to serial
 */
void handleNodeDisconnection(uint8_t nid, uint8_t dir)
{
  disconnectedNodeToSerial(nid, dir);
}

/*
 * Function: handleDataPacket
 * --------------------
 * Handles a data packet from a node by reading it and passing it to serial with crc checksum
 * 
 * payload: the packet
 */
void handleDataPacket(uint8_t *payload)
{
  Data data;
  memcpy(&data, payload, sizeof(data));
  String dataS = dataToString(data);
  toSerialWithCRC(dataS);
}

/*
 * Function: sendNodeInfo
 * --------------------
 * Converts node info to a string and writes it to serial w crc checksum
 * 
 * id: node id
 * nt: node type
 * pos: node position
 * 
 * TODO: String are bad
 */
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

/*
 * Function: runConfiguration
 * --------------------
 * Sets variables and handles start and finish packets for the configuration state
 */
void runConfiguration()
{
  // send own info
  sendNodeInfo(masterId, nt, "-1");

  // Make sure base values are correct, incase running configuration again
  uint8_t packet[1] = {toInitial};
  bus.send_packet_blocking(PJON_BROADCAST, packet, 1); // Make sure nodes also reset their attributes
  interfaceReady = false;
  nextId = 1;
  stackTop = 0;

  packet[0] = start;
  bus.send_packet_blocking(PJON_BROADCAST, packet, 1);

  StackPair stack[maxNodes]; // Use this array like a stack.
  configuration(stack); // Main configuration loop

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

  // Let the nodes know that configuration is done
  packet[0] = csCompleted;
  bus.send_packet_blocking(PJON_BROADCAST, packet, 1);
}

/*
 * Function: configuration
 * --------------------
 * Runs the initial configuration until all directions are checked. 
 * This will assign dynamic ids to each node in the configuration and get their position.
 * More information in the docs.
 * 
 * stack: An array of stackpairs used to determine the positions of the nodes.
 */
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
    { // Node has checked all its directions
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
  { //  Node from stack
    uint8_t content[2] = {dirInstruction, dir};
    bus.send_packet_blocking(stack[stackTop].nodeId, content, 2); // send instruction to Node
    // The Node will react by setting the corresponding pin to high
  }

  uint16_t response = bus.receive(50000); // 0.05 seconds
  if (response == PJON_ACK)
  { // Someone received the data. The receiver should have the isReceiver set to True now
    uint8_t content2[2] = {idPacket, nextId};
    bus.send_packet_blocking(PJON_BROADCAST, content2, 2); // Send the id to the node
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

/*
 * Function: updateCurrentPos
 * --------------------
 * Updates the current position by reading the stack of stackpairs
 * 
 * stack: The stack of stackpairs which containts node id and the current direction checked
 * 
 * TODO currentPos is string... 
 */
void updateCurrentPos(StackPair stack[maxNodes])
{
  currentPos = ""; // Reset
  for (int i = 0; i <= stackTop; i++)
  {
    currentPos += (stack[i].dir);
  }
}

/*
 * Function: sendBounds
 * --------------------
 * Sends a bounds packet to a node
 * 
 * nodeId: To which node the packet should be send
 */
void sendBounds(BoundsData b, uint8_t nodeId)
{
  bus.send_packet_blocking(nodeId, &b, sizeof(b));
}

/*
 * Function: disconnectedNodeToSerial
 * --------------------
 * Converts disconnectedNode data to a string and writes it to serial with crc
 * 
 * found_id: the id of the node that noticed the disconnection
 * dir: the direction of the disconnection
 */
void disconnectedNodeToSerial(uint8_t found_id, uint8_t dir)
{
  String s = String(nodeDisconnected);
  s += " ";
  s += found_id;
  s += ":";
  s += dir;
  toSerialWithCRC(s);
}

/*
 * Function: dataToString
 * --------------------
 * Converts Data type to a string that can be written to serial
 * 
 * data: data to be converted
 * 
 * TODO String are still badc
 */
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

/*
 * Function: toSerialWithCRC
 * --------------------
 * Calculated the CRC checksum of a given string and writes the string and the checksum to serial
 * 
 * s: The string
 * 
 * TODO Strings
 */
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
