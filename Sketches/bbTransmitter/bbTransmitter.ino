#include <PJONSoftwareBitBang.h>
//#include <WebUSB.h> // Arduino leonardo...
//WebUSB WebUSBSerial(1 /* http:// */, "127.0.0.1:5500/JSTesting/Example/index.html");

//#define Serial WebUSBSerial

uint8_t id = 252;

PJONSoftwareBitBang bus(254);
int potPin = 5;

void setup() {
  bus.strategy.set_pin(12);
  bus.begin();
}

void loop() {
//    uint8_t bValue = digitalRead(buttonPin);
//    uint8_t bBontent[3] = {'B', id ,bValue};
//    bus.send_packet(0,bBontent, 3);
    uint16_t potValue = analogRead(potPin);
    uint8_t pContent[4] = {'P', id, (uint8_t)(potValue >> 8),(uint8_t)(potValue & 0xFF)};
    bus.send_packet(0,pContent,4);
};
