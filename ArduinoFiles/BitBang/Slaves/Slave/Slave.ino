#include <ConfiguationFinder.h>
#include <Component.h>
#include <Potentiometer.h>
#include <RotaryEncoder.h>
#include <Button.h>
//#define PJON_INCLUDE_MAC
#include <PJONSoftwareBitBang.h>


// Adjust these values depending on the node
//uint8_t id = 255; // Make sure each node has a different id

const uint8_t potCount = 2; // How many potentiometers are connected to the node
const uint8_t rotCount = 1; // Rotary encoders
const uint8_t buttonCount = 0; // Buttons

const uint8_t potPins[potCount] = {A5, A6}; // The pins the potentiometers are connected
const uint8_t rotPins[rotCount][2] = {{3,4}}; // {{8,9}}; // Rotary encoders
const uint8_t bPins[buttonCount] = {}; // Buttons

// Adjust Until here

Potentiometer pots[potCount];
RotaryEncoder rots[rotCount];
Button buttons[buttonCount];
//const uint8_t compCount =  potCount + rotCount + buttonCount;
//Component components[compCount];

// Communication
uint8_t id = PJON_NOT_ASSIGNED; // Temporary
//uint8_t mac[6] = {1,2,1,2,1,2}; // temporary mac
PJONSoftwareBitBang bus(id);
const uint8_t outputPin = 12;
const uint8_t inputPin = 11;
const uint8_t communicationPin = 12;
const uint8_t master = 254;
bool interfaceReady = false;

ConfigurationFinder cf = ConfigurationFinder(4,5,6,7); // UP, RIGHT, DOWN, LEFT
bool waitingForLight = false;
bool latestReceiver = false;
//// These could be a own thing somewhere
//union Value {
//  uint16_t value;
//  uint8_t values[2];
//};

struct Counts {
  uint8_t rots;
  uint8_t pots;
  uint8_t buttons;
};

struct Data {
  uint16_t value;
  uint8_t id;
  char type;
};

bool sendData(Data data, bool forceSend = false) {
  uint16_t packet = forceSend ? bus.send_packet_blocking(master, &data, sizeof(data)) : bus.send_packet(master, &data, sizeof(data));
  return (packet != PJON_ACK);
}

// Check if potentiometers have changed
void checkPots(bool forceSend = false)
{
  for (int i = 0; i < potCount; i++)
  {
    Potentiometer pot = pots[i];
//    Value value;
//    value.value = pot.getValue();
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


void receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info)
{
  if (char(payload[0]) == 'L'){ // initial LAYOUT
     Serial.println("Received L\nSetting pins to input");
     cf.setPinsInput();
     waitingForLight = true;
  }
  else if (char(payload[0]) == 'I') { // ID
    if (latestReceiver && id == PJON_NOT_ASSIGNED) {
      id = payload[1];
      Serial.println(id);
      bus.set_id(id);
      waitingForLight = false;
      latestReceiver = false;
    }
  }
  else if (char(payload[0]) == 'N') { // New instructions
    // Should check if this node is already gone through all directions, loop situations
    Serial.println("New instructions");
    uint8_t dir = payload[1];
    cf.setPinHigh(dir);
  }
  else if (char(payload[0]) == 'S') // SEND
  {
    delay(random(2000));
    interfaceReady = true;
    checkPots(true);
    checkRots(true);
    checkButtons(true);
  }
  else if (char(payload[0]) == 'Q') { // QUIT
    interfaceReady = false;
  }
  else {
    Serial.println(payload[0]);
  }
}

void randomizeMac(uint8_t mac[]) {
  for (int i = 0; i < 6; ++i) {
    mac[i] = random(256);
  }
}

void setup()
{
  Serial.begin(9600);
  randomSeed(analogRead(0));
//  randomizeMac(mac);
//  bus.set_mac(mac);
  bus.strategy.set_pins(inputPin, outputPin);
  bus.begin();
  bus.set_receiver(receiver_function);

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

void loop()
{
  bus.receive(1500); // It is not guaranteed that slaves will receive the 'Q' command from the master
  if (waitingForLight) {
    if (cf.isAnyPinHigh()) {
      Serial.println("A pin is high");
      latestReceiver = true;
      bus.send_packet_blocking(master, "O", 1);
      bus.receive(2500);
    }
  }
  if (!interfaceReady) { // If not ready just wait until it is
    return;
  }

  checkPots();
  checkRots();
  checkButtons();
//    checkComponents();
};
