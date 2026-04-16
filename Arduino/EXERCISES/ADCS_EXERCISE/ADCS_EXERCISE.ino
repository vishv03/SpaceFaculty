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

#define SAMPLE_TIME_MS 5000   // 5-second sample interval

const float shakeThreshold = 5.0;
bool isShaken = false;

unsigned long lastSunTime = 0;   // enforces 5-second sample time for SUN

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
  obcSerial.listen();

  if (obcSerial.available()) {
    String received = obcSerial.readStringUntil('\n');
    received.trim();

    Serial.print("[OBC CMD] ");
    Serial.println(received);

    if      (received == "SUN")   sunAndMotor();
    else if (received == "ADJ")   { int l=analogRead(LEFT_SENSOR), r=analogRead(RIGHT_SENSOR), b=analogRead(BACK_SENSOR); controlMotor(l,r,b); }
    else if (received == "IMU")   sendIMUData();
    else if (received == "TEMPADCS")  sendTempData();
    else if (received == "SHAKE") shake();
    else {
      obcSerial.println("ERROR UNKNOWN COMMAND: " + received);
      Serial.println("Unknown command received.");
    }
  }
}

// ─────────────────────────────────────────────
// SUN SENSOR + MOTOR CONTROL  (triggered by "SUN")
// ─────────────────────────────────────────────
void sunAndMotor() {

  // Enforce 5-second sample time — reject if called too soon
  if (millis() - lastSunTime < SAMPLE_TIME_MS) {
    unsigned long remaining = (SAMPLE_TIME_MS - (millis() - lastSunTime)) / 1000;
    String busy = "SUN: sample not ready, wait " + String(remaining) + "s";
    obcSerial.println(busy);
    Serial.println("[ADCS] " + busy);
    return;
  }
  lastSunTime = millis();

  // Read all three sensors
  int left  = analogRead(LEFT_SENSOR);
  int right = analogRead(RIGHT_SENSOR);
  int back  = analogRead(BACK_SENSOR);

  Serial.print("[ADCS] L="); Serial.print(left);
  Serial.print(" R=");        Serial.print(right);
  Serial.print(" B=");        Serial.println(back);

  String status = "";

  if (left > right && left > back) {
    // ── Left is brightest → rotate CCW for 1 second ──
    status = "Motor: rotating CCW (left brightest, L=" + String(left) + ")";
    Serial.println("[ADCS] " + status);
    obcSerial.println(status);

    analogWrite(motorPin1, 255);  // CCW: pin1 full, pin2 off
    analogWrite(motorPin2, 0);
    delay(1000);
    analogWrite(motorPin1, 0);    // stop after 1 s
    analogWrite(motorPin2, 0);

  } else if (right > left && right > back) {
    // ── Right is brightest → rotate CW for 1 second ──
    status = "Motor: rotating CW (right brightest, R=" + String(right) + ")";
    Serial.println("[ADCS] " + status);
    obcSerial.println(status);

    analogWrite(motorPin1, 0);    // CW: pin2 full, pin1 off
    analogWrite(motorPin2, 255);
    delay(1000);
    analogWrite(motorPin1, 0);    // stop after 1 s
    analogWrite(motorPin2, 0);

  } else {
    // ── Back is brightest (or tie) → stop motor ──
    status = "Motor: stopped (back brightest, B=" + String(back) + ")";
    Serial.println("[ADCS] " + status);
    obcSerial.println(status);

    analogWrite(motorPin1, 0);
    analogWrite(motorPin2, 0);
  }
}

// ─────────────────────────────────────────────
// TEMPERATURE
// ─────────────────────────────────────────────
void sendTempData() {
  int   tempRaw  = analogRead(TEMP_SENSOR);
  float tempVolt = tempRaw * (5.0 / 1023.0);
  float tempC    = tempVolt * 100.0;

  String reply = "TEMPADCS = " + String(tempC, 1) + " deg";
  obcSerial.println(reply);
  Serial.println("[ADCS→OBC] " + reply);
}

// ─────────────────────────────────────────────
// SUN SENSOR DATA ONLY (raw values, no motor)
// ─────────────────────────────────────────────
void sendSunData(int left, int right, int back) {
  String reply = "L: " + String(left) +
                 " | R: " + String(right) +
                 " | B: " + String(back);
  obcSerial.println(reply);
  Serial.println("[ADCS→OBC] " + reply);
}

// ─────────────────────────────────────────────
// MOTOR CONTROL (standalone, called by ADJ)
// ─────────────────────────────────────────────
void controlMotor(int left, int right, int back) {
  if (left > right && left > back) {
    obcSerial.println("Motor: rotating CCW (left brightest)");
    Serial.println("[ADCS] Motor: rotating CCW");
    analogWrite(motorPin1, 255);
    analogWrite(motorPin2, 0);
    delay(1000);
    analogWrite(motorPin1, 0);
    analogWrite(motorPin2, 0);
  }
  else if (right > left && right > back) {
    obcSerial.println("Motor: rotating CW (right brightest)");
    Serial.println("[ADCS] Motor: rotating CW");
    analogWrite(motorPin1, 0);
    analogWrite(motorPin2, 255);
    delay(1000);
    analogWrite(motorPin1, 0);
    analogWrite(motorPin2, 0);
  }
  else {
    obcSerial.println("Motor: stopped (back brightest)");
    Serial.println("[ADCS] Motor: stopped");
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

// Called by "SUN" — report raw values only
void sendSunValues() {
  int left  = analogRead(LEFT_SENSOR);
  int right = analogRead(RIGHT_SENSOR);
  int back  = analogRead(BACK_SENSOR);
  String reply = "S1: " + String(left) + "\nS2: " + String(right) + "\nS3: " + String(back);
  obcSerial.println(reply);
  Serial.println("[ADCS→OBC] " + reply);
}

// Called by "ADJ" — compare and rotate
void adjMotor() {
  int left  = analogRead(LEFT_SENSOR);
  int right = analogRead(RIGHT_SENSOR);
  int back  = analogRead(BACK_SENSOR);
  controlMotor(left, right, back);  // your existing function
}

// ─────────────────────────────────────────────
// SHAKE DETECTION + MOTOR SEQUENCE
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
    isShaken = false;
  } else if (!shakeDetected) {
    obcSerial.println("SHAKE: No shake detected");
  }
}