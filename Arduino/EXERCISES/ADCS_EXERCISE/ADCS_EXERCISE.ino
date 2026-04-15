#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <SoftwareSerial.h>

Adafruit_BNO055 bno = Adafruit_BNO055(55);

#define RIGHT_SENSOR A0
#define BACK_SENSOR  A2
#define LEFT_SENSOR  A3
#define TEMP_SENSOR  A6
#define motorPin1    9
#define motorPin2    10
#define TX_PIN       2      // ADCS TX → OBC RX (pin 3)
#define RX_PIN       3      // ADCS RX ← OBC TX (pin 4)

const float shakeThreshold = 5.0;
bool isShaken = false;

SoftwareSerial obcSerial(RX_PIN, TX_PIN,true);

// ─────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  obcSerial.begin(9600);
  Serial.println("ADCS online, waiting for OBC commands...");

  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);

  if (!bno.begin()) {
    Serial.println("No BNO055 detected — halting.");
    while (1);
  }
  delay(1000);
}

// ─────────────────────────────────────────────
void loop() {
  if (obcSerial.available()) {
    String received = obcSerial.readStringUntil('\n');
    received.trim();

    Serial.print("[OBC CMD] ");
    Serial.println(received);

    // Read LDR sensors once; share across handlers that need them
    int left  = analogRead(LEFT_SENSOR);
    int right = analogRead(RIGHT_SENSOR);
    int back  = analogRead(BACK_SENSOR);

    // ── Dispatch ─────────────────────────────
    if      (received == "SUN")   sendSunData(left, right, back);
    else if (received == "ADJ")   controlMotor(left, right, back);
    else if (received == "IMU")   sendIMUData();
    else if (received == "TEMP")  sendTempData();      // ← OBC calls this for TEMPADCS
    else if (received == "SHAKE") shake();
    else {
      // Only reaches here when no command matched
      obcSerial.println("ERROR UNKNOWN COMMAND: " + received);
      Serial.println("Unknown command received.");
    }
  }
  // No blanket delay here — let the serial buffer drain naturally
}

// ─────────────────────────────────────────────
// TEMPERATURE — reply label matches OBC command
// ─────────────────────────────────────────────
void sendTempData() {
  int   tempRaw   = analogRead(TEMP_SENSOR);
  float tempVolt  = tempRaw * (5.0 / 1023.0);
  float tempC     = tempVolt * 100.0;           // LM35: 10 mV/°C

  // Label clearly so OBC / LoRa output reads "TEMPADCS = XX.X deg"
  String reply = "TEMPADCS = " + String(tempC, 1) + " deg";
  obcSerial.println(reply);
  Serial.println("[ADCS OBC] " + reply);
}

// ─────────────────────────────────────────────
// SUN SENSOR DATA
// ─────────────────────────────────────────────
void sendSunData(int left, int right, int back) {
  String reply = "L: " + String(left) +
                 " | R: " + String(right) +
                 " | B: " + String(back);
  obcSerial.println(reply);
  Serial.println("[ADCS→OBC] " + reply);
}

// ─────────────────────────────────────────────
// MOTOR CONTROL
// ─────────────────────────────────────────────
void controlMotor(int left, int right, int back) {
  if (left > right && left > back) {
    obcSerial.println("Motor: CCW (left brightest)");
    analogWrite(motorPin1, 255);
    delay(1000);
    analogWrite(motorPin1, 0);
  }
  else if (right > left && right > back) {
    obcSerial.println("Motor: CW (right brightest)");
    analogWrite(motorPin2, 255);
    delay(1000);
    analogWrite(motorPin2, 0);
  }
  else {
    obcSerial.println("Motor: STOP (back brightest)");
    analogWrite(motorPin1, 0);
    analogWrite(motorPin2, 0);
  }
}

// ─────────────────────────────────────────────
// IMU DATA
// ─────────────────────────────────────────────
void sendIMUData() {
  imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
  String reply = "RotX: " + String(euler.x(), 2) +
                 " | RotY: " + String(euler.y(), 2) +
                 " | RotZ: " + String(euler.z(), 2);
  obcSerial.println(reply);
  Serial.println("[ADCS→OBC] " + reply);
}

// ─────────────────────────────────────────────
// SHAKE DETECTION + RESPONSE SEQUENCE
// ─────────────────────────────────────────────
void shake() {
  imu::Vector<3> accel = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);

  bool shakeDetected = (abs(accel.x()) > shakeThreshold ||
                        abs(accel.y()) > shakeThreshold ||
                        abs(accel.z()) > shakeThreshold);

  if (shakeDetected && !isShaken) {
    isShaken = true;
    obcSerial.println("SHAKE: Detected — running sequence");
    Serial.println("[ADCS] Shake detected, running motor sequence.");

    analogWrite(motorPin1, 255); analogWrite(motorPin2, 0);   // CW
    delay(1000);
    analogWrite(motorPin1, 0);   analogWrite(motorPin2, 0);   // stop
    delay(500);
    analogWrite(motorPin1, 0);   analogWrite(motorPin2, 255); // CCW
    delay(1000);
    analogWrite(motorPin1, 0);   analogWrite(motorPin2, 0);   // stop

    obcSerial.println("SHAKE: Sequence complete, awaiting next shake");
    Serial.println("[ADCS] Sequence complete.");
    isShaken = false;   // ready to detect again
  } else if (!shakeDetected) {
    obcSerial.println("SHAKE: No shake detected");
  }
}