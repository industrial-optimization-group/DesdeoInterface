#include <Potentiometer.h>
#include <PJONSoftwareBitBang.h>

uint8_t id = 240; // Make sure each node has different id

PJONSoftwareBitBang bus(id);
int potPin = A5;
Potentiometer pot = Potentiometer(potPin);

void setup() {
  bus.strategy.set_pin(12);
  bus.begin();
}

void loop() {
    uint16_t potValue = pot.getValue();
    uint8_t pContent[4] = {'P', id, (uint8_t)(potValue >> 8),(uint8_t)(potValue & 0xFF)};
    bus.send_packet(0, pContent, 4);
};
