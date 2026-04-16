#include <SPI.h>
#include <LoRa.h>

#ifdef ARDUINO_SAMD_MKRWAN1300
#error "This example is not compatible with the Arduino MKR WAN 1300 board!"
#endif

// ── Valid commands to send to OBC ────────────────
// TEMPOBC       - Request OBC temperature
// TEMPEPS       - Request EPS temperature
// TEMPCOMPARISON- Request temperature comparison
// VBAT          - Request battery voltage
// ALL           - Request full telemetry + status
// TIME          - Request elapsed mission time
// RTIMEOBC      - Reset the OBC mission timer
// STATUS        - Request system status

#define SECRET_KEY "ANGRYPHONES"
String command = "";

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("=== Ground Station LoRa Terminal ===");
  Serial.println("Type a command and press Enter to send.");
  Serial.println("Commands: TEMPOBC, TEMPEPS, TEMPCOMPARISON, VBAT, ALL, TIME, RTIMEOBC, STATUS");
  Serial.println("=====================================");

  if (!LoRa.begin(432E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // Register receive callback for incoming OBC responses
  LoRa.onReceive(onReceive);

  // Start in receive mode
  LoRa.receive();
}

void loop() {
  // Check for a command typed in Serial Monitor
  if (Serial.available() > 0) {
    command = Serial.readStringUntil('\n');
    command.trim();

    if (command.length() > 0) {
      sendCommand(command);
      command = "";
    }
  }
}

// ─────────────────────────────────────────────
// SEND COMMAND TO OBC
// ─────────────────────────────────────────────
void sendCommand(String cmd) {
  String packet = String(SECRET_KEY) + "|" + cmd;



  // Temporarily stop receiving while transmitting
  LoRa.idle();
  
  LoRa.beginPacket();
  LoRa.print(packet);
  LoRa.endPacket();
  Serial.print("[GS] Sending command: ");
  Serial.println(packet);

  // Return to receive mode to listen for OBC response
  LoRa.receive();
}

// ─────────────────────────────────────────────
// RECEIVE CALLBACK — OBC RESPONSE
// ─────────────────────────────────────────────
void onReceive(int packetSize) {
  if (packetSize == 0) return;

  String packet = "";
  while (LoRa.available()) {
    packet += (char)LoRa.read();
  }
  packet.trim();

  Serial.print("[RAW RX] ");
  Serial.println(packet);

  String prefix = String(SECRET_KEY) + "|";
  if(!packet.startsWith(prefix)) {
    Serial.println("[SECURITY] INVALID IGNORE");
    return;
  }

  String incoming = packet.substring(prefix.length());

  Serial.print("[OBC] ");
  Serial.print(incoming);
  Serial.print("  (RSSI: ");
  Serial.print(LoRa.packetRssi());
  Serial.println(" dBm)");
}
