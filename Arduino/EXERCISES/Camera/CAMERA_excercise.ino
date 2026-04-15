#include <Adafruit_VC0706.h>
#include <SoftwareSerial.h>

// Set up SoftwareSerial with the pins requested:
// Tx (Camera) -> Rx (OBC) at D5
// Rx (Camera) -> Tx (OBC) at D6
SoftwareSerial cameraConnection(5, 6);

Adafruit_VC0706 cam = Adafruit_VC0706(&cameraConnection);

void setup() {
  Serial.begin(9600);
  Serial.println("VC0706 Camera Test");

  // Try to locate the camera
  if (cam.begin()) {
    Serial.println("Camera Found:");
  } else {
    Serial.println("No camera found?");
    return;
  }

  // Print out the camera version information (optional)
  char *reply = cam.getVersion();
  if (reply == 0) {
    Serial.print("Failed to get version");
  } else {
    Serial.println("-----------------");
    Serial.print(reply);
    Serial.println("-----------------");
  }


  // Set the image size (640x480, 320x240, or 160x120)
  cam.setImageSize(VC0706_640x480);

  uint8_t imgsize = cam.getImageSize();
  Serial.print("Image size: ");
  if (imgsize == VC0706_640x480) Serial.println("640x480");
  if (imgsize == VC0706_320x240) Serial.println("320x240");
  if (imgsize == VC0706_160x120) Serial.println("160x120");

  Serial.println("Snap in 3 secs...");
  delay(3000);
  
  // Snap a photo
  if (!cam.takePicture()) {
    Serial.println("Failed to snap!");
  } else {
    Serial.println("Picture taken!");

    uint32_t jpglen = cam.frameLength();
    Serial.print("Storing ");
    Serial.print(jpglen, DEC);
    Serial.print(" byte image.");

    while (jpglen > 0) {
      // read 32 bytes at a time;
      uint8_t *buffer;
      uint8_t bytesToRead = min((uint32_t)32, jpglen); // change 32 to 64 for a speedup but may not work with all setups!
      buffer = cam.readPicture(bytesToRead);
      
      for (int i =0; i < bytesToRead; i++) {
        if (buffer[i] < 0x10) Serial.print("0"); //lEading zero for hex digits
        Serial.print(buffer[i], HEX);
        Serial.print("");
      }
      Serial.println();
      jpglen -= bytesToRead;
  }
  Serial.println("End of photo taking");
  }
}

void loop() {
  // Stay idle
}