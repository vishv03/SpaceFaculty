#include <SoftwareSerial.h>
#include <Adafruit_VC0706.h>

SoftwareSerial camSerial(3, 2); // RX, TX
Adafruit_VC0706 cam = Adafruit_VC0706(&camSerial);

void setup() {
  Serial.begin(9600);
  Serial.println("START");

  camSerial.begin(9600);

  if (cam.begin()) {
    Serial.println("CAMERA FOUND");
  } else {
    Serial.println("CAMERA NOT FOUND");
  }
}

void loop() {}