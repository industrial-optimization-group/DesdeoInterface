#include <Potentiometer.h>
#include <RotaryEncoder.h>
#include <Button.h>
#define PJON_INCLUDE_MAC
#include <PJONSoftwareBitBang.h>


// Adjust these values depending on the node
//uint8_t id = 255; // Make sure each node has a different id

const int potCount = 1; // How many potentiometers are connected to the node
const int rotCount = 0; // Rotary encoders
const int buttonCount = 0; // Buttons

const int potPins[potCount] = {A5}; // The pins the potentiometers are connected
const int rotPins[rotCount][2] = {}; // {{8,9}}; // Rotary encoders
const int bPins[buttonCount] = {}; // Buttons

// Adjust Until here

Potentiometer pots[potCount];
RotaryEncoder rots[rotCount];
Button buttons[buttonCount];

// Communication
uint8_t mac[6] = {1,2,1,2,1,2}; // temporary mac
PJONSoftwareBitBang bus(mac);
const int outputPin = 12;
const int inputPin = 11;
const int communicationPin = 12;
const int master = 254;

// Needed for sending data every 2-4 seconds
unsigned long startMillis;
unsigned long currentMillis;


// These could be a own thing somewhere
union Value {
  uint16_t value;
  uint8_t values[2];
};

struct Data {
  Value value;
  uint8_t id;
  char type;
};


// Send potentiometer data to master
//void sendPotData(int potIndex, uint16_t value)
//{
//  Potentiometer pot = pots[potIndex];
//  char type = pot.getType();
//  uint8_t potId = pot.getId();
//  uint8_t content[4] = {type, potId, (uint8_t)(value >> 8), (uint8_t)(value & 0xFF)};
//  bool success = sendToMaster(content, 4);
//  pots[potIndex] = pot;
//}
//
//void sendRotData(int rotIndex, uint8_t values[2])
//{
//  RotaryEncoder rot = rots[rotIndex];
//  uint8_t content[4] = {rot.getType(), rot.getId(), values[0], values[1]};
//  sendToMaster(content, 4);
//  rots[rotIndex] = rot;
//}
//
//void sendButtonData(int bIndex, uint8_t value)
//{
//  Button button = buttons[bIndex];
//  char type = button.getType();
//  uint8_t bId = button.getId();
//  uint8_t content[3] = {type, bId, value};
//  sendToMaster(content, 3);
//  buttons[bIndex] = button;
//}

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
    Value value;
    value.value = pot.getValue();
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
    uint8_t values[2];
    rot.getValues(values);
    Value value;
    value.values[0] = values[0];
    value.values[1] = values[1];
    rots[i] = rot;
    if (rot.hasChanged() || forceSend)
    {
      Data data;
      data.value = value;
      data.id = rot.getId();
      data.type = rot.getType();
      sendData(data, forceSend);
    }
  }
}

void checkButtons(bool forceSend = false)
{
  for (int i = 0; i < buttonCount; i++)
  {
    Button button = buttons[i];
    Value value;
    value.value = button.getValue();
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

void receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info)
{
  if (char(payload[0]) == 'S')
  {
    delay(random(2000));
    checkPots(true);
    checkRots(true);
    checkButtons(true);
  }
  return;
}

void randomizeMac(uint8_t mac[]) {
  for (int i = 0; i < 6; ++i) {
    mac[i] = random(256);
  }
}

void setup()
{
  randomSeed(analogRead(0));
  randomizeMac(mac);
  bus.set_mac(mac);
  bus.strategy.set_pins(inputPin, outputPin);
  bus.begin();
  bus.set_receiver(receiver_function);

  for (int i = 0; i < potCount; i++)
  {
    Potentiometer pot = Potentiometer(potPins[i], i);
    pots[i] = pot;
  }

  for (int i = 0; i < rotCount; i++)
  {
    RotaryEncoder rot = RotaryEncoder(rotPins[i][0], rotPins[i][1], i);
    rots[i] = rot;
  }

  for (int i = 0; i < buttonCount; i++)
  {
    Button button = Button(bPins[i], i);
    buttons[i] = button;
  }

  startMillis = millis();
}

void loop()
{
 // currentMillis = millis();
//  bool forceSend = false;
//  if (currentMillis - startMillis >= (3000 + random(-1000, 1000)))
//  { //send data about every 2-4 seconds no matter what.
//    //forceSend = true;
//    startMillis = millis();
//  }
  checkPots();
  checkRots();
  checkButtons();
  bus.receive(5000);
};
