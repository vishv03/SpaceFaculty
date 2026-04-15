// =============================================
// OBC Serial Plotter Sketch
// No LoRa — Serial Plotter only
// Plots: TEMPOBC, TEMPEPS, VBAT
// Also plots: StdDev of each over a 5s window
// =============================================

#define LM35_OBC A1
#define LM35_EPS A2
#define VBAT_PIN A0

#define SAMPLE_INTERVAL_MS 200             // new plot point every 200ms
#define WINDOW_SECONDS     5
#define MAX_SAMPLES        (WINDOW_SECONDS * 1000 / SAMPLE_INTERVAL_MS)  // = 25

// Rolling buffers for 5s variability calculation
float obcBuffer[MAX_SAMPLES];
float epsBuffer[MAX_SAMPLES];
float vbatBuffer[MAX_SAMPLES];
int   bufIndex = 0;
int   bufCount = 0;

unsigned long lastSample = 0;

// ─────────────────────────────────────────────
// Read helpers — discard first read to let ADC settle
// ─────────────────────────────────────────────
float readTempOBC() {
  analogRead(LM35_OBC);
  delay(10);
  return ((analogRead(LM35_OBC) / 1023.0) * 5.0) * 100.0;
}

float readTempEPS() {
  analogRead(LM35_EPS);
  delay(10);
  return ((analogRead(LM35_EPS) / 1023.0) * 5.0) * 100.0;
}

float readVBAT() {
  analogRead(VBAT_PIN);
  delay(10);
  return (analogRead(VBAT_PIN) / 1023.0) * 5.0;
}

// ─────────────────────────────────────────────
// Standard deviation over the filled buffer
// ─────────────────────────────────────────────
float calcStdDev(float* buf, int count) {
  if (count < 2) return 0.0;

  float sum = 0.0;
  for (int i = 0; i < count; i++) sum += buf[i];
  float mean = sum / count;

  float variance = 0.0;
  for (int i = 0; i < count; i++) {
    float diff = buf[i] - mean;
    variance += diff * diff;
  }
  return sqrt(variance / count);  // population std dev
}

// ─────────────────────────────────────────────
// SETUP
// ─────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Send labels once — Serial Plotter picks them up
  // Format: label: with no space after colon, comma separated
  Serial.println("TEMPOBC:,TEMPEPS:,VBAT:,StdDev_OBC:,StdDev_EPS:,StdDev_VBAT:");
}

// ─────────────────────────────────────────────
// LOOP — runs continuously for the plotter
// ─────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  if (now - lastSample >= SAMPLE_INTERVAL_MS) {
    lastSample = now;

    // Read all three sensors
    float tempOBC = readTempOBC();
    float tempEPS = readTempEPS();
    float vbat    = readVBAT();

    // Store in rolling circular buffer
    obcBuffer[bufIndex]  = tempOBC;
    epsBuffer[bufIndex]  = tempEPS;
    vbatBuffer[bufIndex] = vbat;

    bufIndex = (bufIndex + 1) % MAX_SAMPLES;
    if (bufCount < MAX_SAMPLES) bufCount++;

    // Calculate variability (std dev) over last 5 seconds
    float sdOBC  = calcStdDev(obcBuffer,  bufCount);
    float sdEPS  = calcStdDev(epsBuffer,  bufCount);
    float sdVBAT = calcStdDev(vbatBuffer, bufCount);

    // Output one line — Serial Plotter draws one point per println
    Serial.print("TEMPOBC:");    Serial.print(tempOBC, 2); Serial.print(",");
    Serial.print("TEMPEPS:");    Serial.print(tempEPS, 2); Serial.print(",");
    Serial.print("VBAT:");       Serial.print(vbat, 3);    Serial.print(",");
    Serial.print("StdDev_OBC:"); Serial.print(sdOBC, 3);   Serial.print(",");
    Serial.print("StdDev_EPS:"); Serial.print(sdEPS, 3);   Serial.print(",");
    Serial.print("StdDev_VBAT:"); Serial.println(sdVBAT, 4);
  }
}
