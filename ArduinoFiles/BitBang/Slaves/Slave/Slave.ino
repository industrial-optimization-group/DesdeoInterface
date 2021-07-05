#include <ConfiguationFinder.h>
#include <Component.h>
#include <Potentiometer.h>
#include <RotaryEncoder.h>
#include <Button.h>
#include <PJONSoftwareBitBang.h>

// Adjust these values depending on the node
const uint8_t potCount = 1;    // How many potentiometers are connected to the node
const uint8_t rotCount = 0;    // Rotary encoders
const uint8_t buttonCount = 0; // Buttons

const uint8_t potPins[potCount] = {A5};  // The pins the potentiometers are connected
const uint8_t rotPins[rotCount][2] = {}; // {{8,9}}; // Rotary encoders
const uint8_t bPins[buttonCount] = {};   // Buttons
// Adjust Until here

Potentiometer pots[potCount];
RotaryEncoder rots[rotCount];
Button buttons[buttonCount];
//const uint8_t compCount =  potCount + rotCount + buttonCount;
//Component components[compCount];

// Communication
uint8_t id = PJON_NOT_ASSIGNED; // Temporary id, until gets assigned a unique id by the master
PJONSoftwareBitBang bus(id);
const uint8_t outputPin = 12;
const uint8_t inputPin = 11;
const uint8_t master = 254;
bool interfaceReady = false;

// For alive messages
bool directionsToCheck[4];
unsigned long lastAliveSent;
int aliveSendInterval = 500; //milliseconds + random(200)

// For setting direction pins and configuration
ConfigurationFinder cf = ConfigurationFinder(4, 5, 6, 7); // UP, RIGHT, DOWN, LEFT
bool waitingForLight = false;
bool latestReceiver = false;

// This is the type of data that is send to the master
struct Data
{
  uint8_t nodeId;
  uint16_t value;
  uint8_t id;
  char type;
};

// Send the Data struct to master
bool sendData(Data data, bool forceSend = false)
{
  data.nodeId = id;
  uint16_t packet = forceSend ? bus.send_packet_blocking(master, &data, sizeof(data)) : bus.send_packet(master, &data, sizeof(data));
  return (packet != PJON_ACK);
}

// Check if potentiometers have changed
void checkPots(bool forceSend = false)
{
  for (int i = 0; i < potCount; i++)
  {
    Potentiometer pot = pots[i];
    uint16_t value = pot.getValue();
    pots[i] = pot;
    if (pot.hasChanged() || forceSend)
    {
      Data data;
      data.value = value;
      data.id = pot.getId();
      data.type = pot.getType();
      sendData(data, forceSend);
    }
  }
};

void checkRots(bool forceSend = false)
{
  for (int i = 0; i < rotCount; i++)
  {
    RotaryEncoder rot = rots[i];
    uint16_t value = rot.getValue();
    rots[i] = rot;
    if (rot.hasChanged() || forceSend)
    {
      Data data;
      data.value = value;
      data.id = rot.getId();
      data.type = rot.getType();
      sendData(data, true);
    }
  }
}

void checkButtons(bool forceSend = false)
{
  for (int i = 0; i < buttonCount; i++)
  {
    Button button = buttons[i];
    //    Value value;
    //    value.value = button.getValue();
    uint16_t value = button.getValue();
    buttons[i] = button;
    if (button.hasChanged() || forceSend)
    {
      Data data;
      data.value = value;
      data.id = button.getId();
      data.type = button.getType();
      sendData(data, forceSend);
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
}

// This function is called whenever a packet is received
void receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info)
{
  if (char(payload[0]) == 'L')
  { // initial LAYOUT
    Serial.println("L");
    cf.setPinsInput();
    waitingForLight = true;
  }
  else if (char(payload[0]) == 'D') {
    uint8_t dir = payload[1];
    Serial.println(dir);
    directionsToCheck[dir] = true;
  }
  else if (char(payload[0]) == 'F')
  { // Reset everything
    Serial.println("F");
    setSelfToInitialMode();
  }
  else if (char(payload[0]) == 'I' && id == PJON_NOT_ASSIGNED)
  { // ID
    if (latestReceiver)
    {
      Serial.println("Received id");
      latestReceiver = false;
      id = payload[1];
      bus.set_id(id);
      waitingForLight = false;
      cf.setPinsInput(); // Set back to input mode. In case disconnecting happens
    }
  }
  else if (char(payload[0]) == 'N')
  { // New instructions
    uint8_t dir = payload[1];
    Serial.println("Received direction");
    cf.setPinsInput(); // Reset pins first
    cf.setPinHigh(dir);
  }
  else if (char(payload[0]) == 'S') 
  { // SEND data, interface is ready and configured
    //lastAliveSent = millis();
    setDirectionPins();
    interfaceReady = true;
  }
  else if (char(payload[0]) == 'Q')
  { // QUIT
    interfaceReady = false;
  }
  else
  {
    return;
  }
}

void setDirectionPins() {
  cf.setPinsInput();
  for (int i = 0; i < 4; ++i) {
    if (!directionsToCheck[i]) {
      Serial.print("Setting pin ");
      Serial.println(i);
      cf.setPinHigh(i);
    }
  }
}

void setup()
{
  Serial.begin(9600);
  randomSeed(analogRead(A0));
  aliveSendInterval += random(200);
  bus.strategy.set_pins(inputPin, outputPin);
  bus.begin();
  bus.set_receiver(receiver_function);
  bus.send_packet_blocking(master, "H", 1); 

  for (int i = 0; i < potCount; i++)
  {
    Potentiometer pot = Potentiometer(potPins[i], i);
    pots[i] = pot;
    //components[i] = pot;
  }

  for (int i = 0; i < rotCount; i++)
  {
    RotaryEncoder rot = RotaryEncoder(rotPins[i][0], rotPins[i][1], i);
    rots[i] = rot;
    //components[potCount + i] = rot;
  }

  for (int i = 0; i < buttonCount; i++)
  {
    Button button = Button(bPins[i], i);
    buttons[i] = button;
    //components[potCount + rotCount + i] = button;
  }
}

void sendAliveMsg()
{
  uint8_t content[2] = {'A', id};
  bus.send_packet_blocking(master, content, 2);
}

int8_t checkNodes() {
  for (int i = 0; i < 4; ++i) {
    if (directionsToCheck[i]) {
      if (!cf.isPinHigh(i)) return i;
    }
  }
  return -1;
}

void sendComponentInfo()
{
  uint8_t content[4] = {'C', potCount, rotCount, buttonCount};
  bus.send_packet_blocking(master, content, 4);
}

void loop()
{
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
//  
//  if (millis() - lastAliveSent > aliveSendInterval)
//  {
//    lastAliveSent = millis();
//    sendAliveMsg();
//  } 
  bus.receive(1500);
  checkPots(); //ADC
  bus.receive(1500);
  checkRots(); //Interupts
  checkButtons();

    int8_t disconnectedNode = checkNodes();
    if (disconnectedNode >= 0) {
      directionsToCheck[disconnectedNode] = false;
      uint8_t content[3] = {'D', id, disconnectedNode};
      bus.send_packet_blocking(master, content, 3);
  }
  //    checkComponents();
};
