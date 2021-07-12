#include <Datatypes.h>
#include <EEPROM.h>
#include <ConfiguationFinder.h>
#include <Component.h>
#include <Potentiometer.h>
#include <RotaryEncoder.h>
#include <Button.h>
#include <PJONSoftwareBitBang.h>
#define SERIAL_SIZE 30

struct MyEeprom 
{
  bool configured;
  bool isMaster;
  uint8_t type;
};

node n;
bool isMaster = false; // TODO check if multiple masters?
bool configured = false;

// max 3 of each
const uint8_t maxComponents = 3;
const uint8_t potPins[maxComponents] = {A0, A2, A3};  // The pins the potentiometers are connected
const uint8_t rotPins[maxComponents][2] = {{2,3}, {4,5}, {6,7}};
const uint8_t bPins[maxComponents] = {8,9,10};   // Buttons

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
const uint8_t master = 254;
bool interfaceReady = false; // true // -> false?

// Direction where a node is
bool directionsToCheck[4];

// For setting direction pins and configuration
ConfigurationFinder cf = ConfigurationFinder(7,15,14,16); // UP, RIGHT, DOWN, LEFT
//ConfigurationFinder cf = ConfigurationFinder(7,5,6,9);  // for nanos
bool waitingForLight = false;
bool latestReceiver = false;

// This is the data structure that is send to the master
struct Data
{
  uint8_t nodeId;
  double value;
  uint8_t id;
  char type;
};

struct Bounds 
{
  char componentType;
  uint8_t componentId;
  double minValue;
  double maxValue;
  double stepSize;
};

const char configure = 'F';
const char nodeInfo = 'N';
const char nodeConnected = 'C'; 
const char nodeDisconnected = 'D';
const char componentValue = 'V';
const char start = 'S';
const char quit = 'Q';
const char reset = 'E';
const char idPacket = 'I';
const char dirInstruction = 'N';
const char csCompleted = 'O';
const char dirToCheck = 'D';


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

void saveValues(bool isMaster, uint8_t type)
{
  MyEeprom epr;
  epr.configured = true;
  epr.isMaster = isMaster;
  // TODO validate type
  epr.type = type;
  EEPROM.put(0, epr);
  Serial.println("Configuration saved");
  loadValues();
}

// Send the Data struct to master
bool sendData(Data data, bool blocking = false)
{
  data.nodeId = id;
  uint16_t packet = blocking ? bus.send_packet_blocking(master, &data, sizeof(data)) : bus.send_packet(master, &data, sizeof(data));
  return (packet != PJON_ACK);
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
      sendData(data, blocking);
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
      sendData(data, blocking);
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
      sendData(data, blocking);
    }
  }
}

// I WOULD LIKE SOMETHING LIKE THIS
//void checkComponents(bool forceSend = false) {
//  for (int i = 0; i < compCount; i++)
//  {
//    Component comp = components[i];
//    uint16_t value = comp.getValue();
//    components[i] = comp;
//    if (comp.hasChanged() || forceSend)
//    {
//      Data data;
//      data.value = value;
//      data.id = comp.getId();
//      data.type = comp.getType();
//      sendData(data, forceSend);
//    }
//  }
//}

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

// This function is called whenever a packet is received
void receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info)
{
  Serial.println(char(payload[0]));
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
    Bounds bounds;
    memcpy(&bounds, payload, sizeof(bounds));
    if (bounds.componentType == 'R') {
      rots[bounds.componentId].setBounds(bounds.minValue, bounds.maxValue, bounds.stepSize);
    }
    else if (bounds.componentType == 'P') {
      pots[bounds.componentId].setBounds(bounds.minValue, bounds.maxValue);
    }
  }
}

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

void setup()
{
  Serial.begin(9600);
  loadValues();
  bus.strategy.set_pins(inputPin, outputPin);
  bus.begin();
  bus.set_receiver(receiver_function);
  uint8_t packet[1] = {nodeConnected};
  bus.send_packet_blocking(master, packet, 1); 

  //initializeComponents();
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

// Send initial info
void sendComponentInfo()
{
  uint8_t content[4] = {nodeInfo, n.potCount, n.rotCount, n.butCount};
  bus.send_packet_blocking(master, content, 4);
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
  if (!interfaceReady)
  {
    if (waitingForLight)
    {
      if (cf.isAnyPinHigh())
      {
        latestReceiver = true;
        sendComponentInfo();
        bus.receive(2500);
      }
    }
    return;
  }

  bus.receive(1500);
  checkPots(); //ADC
  bus.receive(1500);
  checkRots(); //Interupts
  checkButtons();

    int8_t disconnectedNode = checkNodes();
    if (disconnectedNode >= 0) {
      directionsToCheck[disconnectedNode] = false;
      uint8_t content[3] = {nodeDisconnected, id, disconnectedNode};
      bus.send_packet_blocking(master, content, 3);
  }
};
