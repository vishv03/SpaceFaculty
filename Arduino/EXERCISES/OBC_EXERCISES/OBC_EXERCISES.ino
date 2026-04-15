#include <SPI.h>
#include <LoRa.h>
#include <SoftwareSerial.h>

// ── Pin Definitions ──────────────────────────
#define LM35_OBC A1
#define LM35_EPS A2
#define VBAT_PIN A0
#define RX_PIN 3
#define TX_PIN 4

#define GROUP_NAME "TEAM4"

// ── Shared Cached Sensor Readings ────────────
float g_tempOBC = 0.0;
float g_tempEPS = 0.0;
float g_vbat    = 0.0;
unsigned long lastReadTime = 0;
#define READ_INTERVAL_MS 500

bool sensorsUpdated = false;

// ── State Variables ──────────────────────────
unsigned long startTime = 0;
bool obcWarningSent = false;
bool epsWarningSent = false;
bool vbatWarningSet = false;

// ─────────────────────────────────────────────
// SETUP
// ─────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  link.begin(9600);
  while (!Serial);

  Serial.println("=== OBC Booting ===");

  if (!LoRa.begin(432E6)) {
    Serial.println("LoRa failed!");
    while (1);
  }

  // Register receive callback for incoming GS commands
  LoRa.onReceive(onReceive);

  // Start in receive mode waiting for GS commands
  LoRa.receive();

  startTime = millis();
  Serial.println("OBC Ready. Listening for commands...");
}

// ─────────────────────────────────────────────
// LOOP
// ─────────────────────────────────────────────
void loop() {
  updateSensors();
  checkTemperatureWarning();
  checkVoltageWarning();
}

// ─────────────────────────────────────────────
// RECEIVE CALLBACK — GS COMMAND HANDLER
// ─────────────────────────────────────────────
void onReceive(int packetSize) {
  if (packetSize == 0) return;

  String command = "";
  while (LoRa.available()) {
    command += (char)LoRa.read();
  }
  command.trim();

  Serial.print("[GS CMD] Received: ");
  Serial.println(command);

  // Must go idle before transmitting a response
  LoRa.idle();

  if (command == "TEMPOBC") {
    sendTempOBC();
  } else if (command == "TEMPEPS") {
    sendTempEPS();
  } else if (command == "TEMPCOMPARISON") {
    sendMaxTemp();
  } else if (command == "VBAT") {
    sendVBAT();
  } else if (command == "ALL") {
    sendAll();
  } else if (command == "TIME") {
    sendTime();
  } else if (command == "RTIMEOBC") {
    resetTime();
    sendAck("TIME RESET OK");
  } else if (command == "STATUS") {
    sendStatus();
  } else if (command == "TEMPADCS") {
    retrieveTempADCS();
  }else {
    sendAck("UNKNOWN CMD: " + command);
  }

  // Return to receive mode after responding
  LoRa.receive();
}

// ─────────────────────────────────────────────
// SENSOR UPDATE
// ─────────────────────────────────────────────
float averageRead(int pin, int samples) {
  long total = 0;
  for (int i = 0; i < samples; i++) {
    analogRead(pin); // discard first read to let ADC settle
    delay(2);
    total += analogRead(pin);
    delay(2);
  }
  return (total / (float)samples / 1023.0) * 5.0 * 100.0;
}

float averageReadVolts(int pin, int samples) {
  long total = 0;
  for (int i = 0; i < samples; i++) {
    analogRead(pin); // discard first read to let ADC settle
    delay(2);
    total += analogRead(pin);
    delay(2);
  }
  return (total / (float)samples / 1023.0) * 5.0;
}

void updateSensors() {
  sensorsUpdated = false;
  if (millis() - lastReadTime < READ_INTERVAL_MS) return;
  lastReadTime = millis();

  g_tempOBC = averageRead(LM35_OBC, 5);
  g_tempEPS = averageRead(LM35_EPS, 5);
  g_vbat    = averageReadVolts(VBAT_PIN, 5);

  // Reset warning flags once sensors recover
  if (g_tempOBC <= 30) obcWarningSent = false;
  if (g_tempEPS <= 30) epsWarningSent = false;
  if (g_vbat >= 3.6)   vbatWarningSet = false;

  sensorsUpdated = true;
}

// ─────────────────────────────────────────────
// HELPER — SEND LORA RESPONSE
// ─────────────────────────────────────────────
void sendLoRa(String msg) {
  LoRa.beginPacket();
  LoRa.print(msg);
  LoRa.endPacket();

  Serial.print("[TX] ");
  Serial.println(msg);
}

void sendAck(String msg) {
  sendLoRa("[" + String(GROUP_NAME) + "] " + msg);
}

// ─────────────────────────────────────────────
// TIME
// ─────────────────────────────────────────────
void resetTime() {
  startTime = millis();
  Serial.println("Mission timer reset.");
}

void getTimeString(String &out) {
  unsigned long elapsed = (millis() - startTime) / 1000;
  int minutes = elapsed / 60;
  int seconds = elapsed % 60;
  out = String(minutes) + "m " + String(seconds) + "s";
}

void sendTime() {
  String t;
  getTimeString(t);
  sendLoRa("[" + String(GROUP_NAME) + "] TIME = " + t);
}

// ─────────────────────────────────────────────
// TEMPERATURE — OBC
// ─────────────────────────────────────────────
void sendTempOBC() {
  sendLoRa("[" + String(GROUP_NAME) + "] TEMPOBC = " + String(g_tempOBC, 1) + " deg");
}

// ─────────────────────────────────────────────
// TEMPERATURE — EPS
// ─────────────────────────────────────────────
void sendTempEPS() {
  sendLoRa("[" + String(GROUP_NAME) + "] TEMPEPS = " + String(g_tempEPS, 1) + " deg");
}

void retrieveTempADCS() {
  
  // Read from ADCS PIN
  // Serial.read(ADCS_TEMP_PIN)

  link.println("TEMPADCS");
  Serial.println("Request sent to ADCS...")

  if (link.available())
}

// ─────────────────────────────────────────────
// TEMPERATURE COMPARISON
// ─────────────────────────────────────────────
void sendMaxTemp() {
  String msg = "[" + String(GROUP_NAME) + "] ";
  if (g_tempEPS > g_tempOBC) {
    msg += "TEMPEPS > TEMPOBC by " + String(g_tempEPS - g_tempOBC, 2) + " deg";
  } else if (g_tempOBC > g_tempEPS) {
    msg += "TEMPOBC > TEMPEPS by " + String(g_tempOBC - g_tempEPS, 2) + " deg";
  } else {
    msg += "TEMPOBC == TEMPEPS (" + String(g_tempOBC, 1) + " deg)";
  }
  sendLoRa(msg);
}

// ─────────────────────────────────────────────
// VBAT
// ─────────────────────────────────────────────
void sendVBAT() {
  sendLoRa("[" + String(GROUP_NAME) + "] VBAT = " + String(g_vbat, 2) + " V");
}

// ─────────────────────────────────────────────
// SEND ALL
// ─────────────────────────────────────────────
void sendAll() {
  String t;
  getTimeString(t);

  String msg1 = "[" + String(GROUP_NAME) + "] ";
  msg1 += "TEMPOBC = " + String(g_tempOBC, 2) + " deg | ";
  msg1 += "TEMPEPS = " + String(g_tempEPS, 2) + " deg | ";
  msg1 += "VBAT = "    + String(g_vbat, 2)    + " V | ";
  msg1 += "TIME = "    + t;

  String msg2 = "[" + String(GROUP_NAME) + "] STATUS = ";
  if (g_tempOBC > 30) {
    msg2 += "WARNING! TEMPOBC too high: " + String(g_tempOBC, 0) + " deg";
  } else if (g_tempEPS > 30) {
    msg2 += "WARNING! TEMPEPS too high: " + String(g_tempEPS, 0) + " deg";
  } else if (g_vbat < 3.6) {
    msg2 += "WARNING! VBAT too low: " + String(g_vbat, 2) + " V";
  } else {
    msg2 += "NOMINAL";
  }

  sendLoRa(msg1);
  delay(100); // small gap between two packets
  sendLoRa(msg2);
}

// ─────────────────────────────────────────────
// STATUS
// ─────────────────────────────────────────────
void sendStatus() {
  String t;
  getTimeString(t);

  String msg = "[" + String(GROUP_NAME) + "] [" + t + "] STATUS = ";
  if (g_tempOBC > 30) {
    msg += "WARNING! TEMPOBC too high: " + String(g_tempOBC, 0) + " deg";
  } else if (g_tempEPS > 30) {
    msg += "WARNING! TEMPEPS too high: " + String(g_tempEPS, 0) + " deg";
  } else if (g_vbat < 3.6) {
    msg += "WARNING! VBAT too low: " + String(g_vbat, 2) + " V";
  } else {
    msg += "NOMINAL";
  }
  sendLoRa(msg);
}

// ─────────────────────────────────────────────
// AUTOMATIC WARNINGS (runs every loop)
// ─────────────────────────────────────────────
void checkTemperatureWarning() {
  if (!sensorsUpdated) return;

  String t;
  getTimeString(t);

  if (g_tempOBC > 30 && !obcWarningSent) {
    String msg = "[" + String(GROUP_NAME) + "] [" + t + "] AUTO-WARN: TEMPOBC too high: " + String(g_tempOBC, 0) + " deg";
    LoRa.idle();
    sendLoRa(msg);
    LoRa.receive();
    Serial.println(msg);
    obcWarningSent = true;
  }

  if (g_tempEPS > 30 && !epsWarningSent) {
    String msg = "[" + String(GROUP_NAME) + "] [" + t + "] AUTO-WARN: TEMPEPS too high: " + String(g_tempEPS, 0) + " deg";
    LoRa.idle();
    sendLoRa(msg);
    LoRa.receive();
    Serial.println(msg);
    epsWarningSent = true;
  }
}

void checkVoltageWarning() {
  if (!sensorsUpdated) return;

  String t;
  getTimeString(t);

  if (g_vbat < 3.6 && !vbatWarningSet) {
    String msg = "[" + String(GROUP_NAME) + "] [" + t + "] AUTO-WARN: VBAT too low: " + String(g_vbat, 2) + " V";
    LoRa.idle();
    sendLoRa(msg);
    LoRa.receive();
    Serial.println(msg);
    vbatWarningSet = true;
  }
}
