#include <SPI.h>
#include <LoRa.h>
#include <SoftwareSerial.h>

// ── Pin Definitions ──────────────────────────
#define LM35_OBC  A1
#define LM35_EPS  A2
#define VBAT_PIN  A0
#define TX_PIN    4
#define RX_PIN    3

SoftwareSerial adcs(RX_PIN, TX_PIN,true);   // OBC RX=3 ← ADCS TX(2),  OBC TX=4 → ADCS RX(3)

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

// ── Forward Declarations ─────────────────────
void handleCommand(String command);
void sendTempOBC();
void sendTempEPS();
void sendMaxTemp();
void sendVBAT();
void sendAll();
void sendTime();
void resetTime();
void sendStatus();
void retrieveTempADCS();
void sendLoRa(String msg);
void sendAck(String msg);
void getTimeString(String &out);
void updateSensors();
void checkTemperatureWarning();
void checkVoltageWarning();
float averageRead(int pin, int samples);
float averageReadVolts(int pin, int samples);

// ─────────────────────────────────────────────
// SETUP
// ─────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  adcs.begin(9600);
  while (!Serial);

  Serial.println("=== OBC Booting ===");

  if (!LoRa.begin(432E6)) {
    Serial.println("LoRa failed!");
    while (1);
  }

  LoRa.onReceive(onReceive);
  LoRa.receive();

  startTime = millis();
  Serial.println("OBC Ready. Type a command (e.g. TEMPADCS) into Serial Monitor, or wait for LoRa commands.");
}

// ─────────────────────────────────────────────
// LOOP
// ─────────────────────────────────────────────
void loop() {
  updateSensors();
  checkTemperatureWarning();
  checkVoltageWarning();

  // ── Serial Monitor command input ──────────
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() > 0) {
      Serial.print("[Serial Monitor CMD] ");
      Serial.println(cmd);
      LoRa.idle();
      handleCommand(cmd);   // shared dispatcher
      LoRa.receive();
    }
  }
}

// ─────────────────────────────────────────────
// LORA RECEIVE CALLBACK
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

  LoRa.idle();
  handleCommand(command);
  LoRa.receive();
}

// ─────────────────────────────────────────────
// SHARED COMMAND DISPATCHER
// ─────────────────────────────────────────────
void handleCommand(String command) {
  if      (command == "TEMPOBC")        sendTempOBC();
  else if (command == "TEMPEPS")        sendTempEPS();
  else if (command == "TEMPCOMPARISON") sendMaxTemp();
  else if (command == "VBAT")           sendVBAT();
  else if (command == "ALL")            sendAll();
  else if (command == "TIME")           sendTime();
  else if (command == "RTIMEOBC")       { resetTime(); sendAck("TIME RESET OK"); }
  else if (command == "STATUS")         sendStatus();
  else if (command == "TEMPADCS")       retrieveTempADCS();   // ← your target command
  else                                  sendAck("UNKNOWN CMD: " + command);
}

// ─────────────────────────────────────────────
// SENSOR HELPERS
// ─────────────────────────────────────────────
float averageRead(int pin, int samples) {
  long total = 0;
  for (int i = 0; i < samples; i++) {
    analogRead(pin);
    delay(2);
    total += analogRead(pin);
    delay(2);
  }
  return (total / (float)samples / 1023.0) * 5.0 * 100.0;
}

float averageReadVolts(int pin, int samples) {
  long total = 0;
  for (int i = 0; i < samples; i++) {
    analogRead(pin);
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

  if (g_tempOBC <= 30) obcWarningSent = false;
  if (g_tempEPS <= 30) epsWarningSent = false;
  if (g_vbat    >= 3.6) vbatWarningSet = false;

  sensorsUpdated = true;
}

// ─────────────────────────────────────────────
// LORA SEND HELPERS
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
  out = String(elapsed / 60) + "m " + String(elapsed % 60) + "s";
}

void sendTime() {
  String t; getTimeString(t);
  sendLoRa("[" + String(GROUP_NAME) + "] TIME = " + t);
}

// ─────────────────────────────────────────────
// TEMPERATURE
// ─────────────────────────────────────────────
void sendTempOBC() {
  sendLoRa("[" + String(GROUP_NAME) + "] TEMPOBC = " + String(g_tempOBC, 1) + " deg");
}

void sendTempEPS() {
  sendLoRa("[" + String(GROUP_NAME) + "] TEMPEPS = " + String(g_tempEPS, 1) + " deg");
}

void sendMaxTemp() {
  String msg = "[" + String(GROUP_NAME) + "] ";
  if      (g_tempEPS > g_tempOBC) msg += "TEMPEPS > TEMPOBC by " + String(g_tempEPS - g_tempOBC, 2) + " deg";
  else if (g_tempOBC > g_tempEPS) msg += "TEMPOBC > TEMPEPS by " + String(g_tempOBC - g_tempEPS, 2) + " deg";
  else                             msg += "TEMPOBC == TEMPEPS (" + String(g_tempOBC, 1) + " deg)";
  sendLoRa(msg);
}

// ─────────────────────────────────────────────
// RETRIEVE ADCS TEMPERATURE  ← core new function
// ─────────────────────────────────────────────
void retrieveTempADCS() {
  Serial.println("[ADCS] Sending TEMP request to ADCS...");

  // Send the command that the ADCS sketch recognises
  adcs.println("TEMP");

  unsigned long start = millis();
  String response = "";

  // Block for up to 2 s waiting for the ADCS reply
  while (millis() - start < 2000) {
    if (adcs.available()) {
      response = adcs.readStringUntil('\n');
      response.trim();
      break;
    }
  }

  if (response.length() > 0) {
    Serial.print("[ADCS] Response: ");
    Serial.println(response);
    sendLoRa("[" + String(GROUP_NAME) + "] TEMPADCS: " + response);
  } else {
    Serial.println("[ADCS] No response within timeout.");
    sendLoRa("[" + String(GROUP_NAME) + "] ERROR: No response from ADCS");
  }
}

// ─────────────────────────────────────────────
// VBAT / ALL / STATUS
// ─────────────────────────────────────────────
void sendVBAT() {
  sendLoRa("[" + String(GROUP_NAME) + "] VBAT = " + String(g_vbat, 2) + " V");
}

void sendAll() {
  String t; getTimeString(t);
  String msg1 = "[" + String(GROUP_NAME) + "] TEMPOBC = " + String(g_tempOBC, 2) +
                " deg | TEMPEPS = " + String(g_tempEPS, 2) +
                " deg | VBAT = "    + String(g_vbat, 2) +
                " V | TIME = "      + t;
  String msg2 = "[" + String(GROUP_NAME) + "] STATUS = ";
  if      (g_tempOBC > 30)  msg2 += "WARNING! TEMPOBC too high: " + String(g_tempOBC, 0) + " deg";
  else if (g_tempEPS > 30)  msg2 += "WARNING! TEMPEPS too high: " + String(g_tempEPS, 0) + " deg";
  else if (g_vbat < 3.6)    msg2 += "WARNING! VBAT too low: "     + String(g_vbat, 2)    + " V";
  else                       msg2 += "NOMINAL";
  sendLoRa(msg1);
  delay(100);
  sendLoRa(msg2);
}

void sendStatus() {
  String t; getTimeString(t);
  String msg = "[" + String(GROUP_NAME) + "] [" + t + "] STATUS = ";
  if      (g_tempOBC > 30)  msg += "WARNING! TEMPOBC too high: " + String(g_tempOBC, 0) + " deg";
  else if (g_tempEPS > 30)  msg += "WARNING! TEMPEPS too high: " + String(g_tempEPS, 0) + " deg";
  else if (g_vbat < 3.6)    msg += "WARNING! VBAT too low: "     + String(g_vbat, 2)    + " V";
  else                       msg += "NOMINAL";
  sendLoRa(msg);
}

// ─────────────────────────────────────────────
// AUTO-WARNINGS
// ─────────────────────────────────────────────
void checkTemperatureWarning() {
  if (!sensorsUpdated) return;
  String t; getTimeString(t);
  if (g_tempOBC > 30 && !obcWarningSent) {
    String msg = "[" + String(GROUP_NAME) + "] [" + t + "] AUTO-WARN: TEMPOBC too high: " + String(g_tempOBC, 0) + " deg";
    LoRa.idle(); sendLoRa(msg); LoRa.receive();
    obcWarningSent = true;
  }
  if (g_tempEPS > 30 && !epsWarningSent) {
    String msg = "[" + String(GROUP_NAME) + "] [" + t + "] AUTO-WARN: TEMPEPS too high: " + String(g_tempEPS, 0) + " deg";
    LoRa.idle(); sendLoRa(msg); LoRa.receive();
    epsWarningSent = true;
  }
}

void checkVoltageWarning() {
  if (!sensorsUpdated) return;
  String t; getTimeString(t);
  if (g_vbat < 3.6 && !vbatWarningSet) {
    String msg = "[" + String(GROUP_NAME) + "] [" + t + "] AUTO-WARN: VBAT too low: " + String(g_vbat, 2) + " V";
    LoRa.idle(); sendLoRa(msg); LoRa.receive();
    vbatWarningSet = true;
  }
}