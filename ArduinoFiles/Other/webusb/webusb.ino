#include <WebUSB.h>

/**
 * Creating an instance of WebUSBSerial will add an additional USB interface to
 * the device that is marked as vendor-specific (rather than USB CDC-ACM) and
 * is therefore accessible to the browser.
 *
 * The URL here provides a hint to the browser about what page the user should
 * navigate to to interact with the device.
 */
WebUSB WebUSBSerial(1 /* http:// */, "127.0.0.1:5500/JSTesting/Example/index.html");

#define Serial WebUSBSerial

const int potPin = A5;
const int bPin = 2;
int value = 0;
void setup() {
  while (!Serial) {
    ;
  }
  pinMode(bPin, INPUT);
  Serial.begin(9600);
  Serial.write("Sketch begins.\r\n");
  Serial.flush();

}



void loop() {
  if (Serial) {
      value = analogRead(potPin);
      String valueS = String(value);
      Serial.write("POT");
      for (int i = 0; i < valueS.length(); i++)
        Serial.write(valueS[i]);
      Serial.write("\r\n");
      Serial.flush();
      Serial.write("BUT");
      if (digitalRead(bPin))
        Serial.write("DOWN\r\n");
    }
       Serial.flush();
    delay(100);
}
