#include <RotaryEncoder.h>

#include <PJONSoftwareBitBang.h>

uint8_t id = 251; // Make sure each node has different id

PJONSoftwareBitBang bus(id);
RotaryEncoder rot = RotaryEncoder(8,9);

void setup() {
  bus.strategy.set_pin(12);
  bus.begin();
}

void loop() {
    uint8_t values[2];
    rot.getValues(values);
    uint8_t pContent[4] = {'R', id, values[0], values[0]};
    bus.send_packet(0, pContent, 4);
};
