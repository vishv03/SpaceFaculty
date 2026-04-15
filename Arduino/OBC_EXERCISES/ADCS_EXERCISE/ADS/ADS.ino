#define RIGHT_SENSOR A0
#define BACK_SENSOR  A2 
#define LEFT_SENSOR  A3 
int motorPin = 9;

void setup() {
  Serial.begin(9600);
  pinMode(motorPin, OUTPUT);
}

void loop() {

  int left  = analogRead(LEFT_SENSOR);
  int right = analogRead(RIGHT_SENSOR);
  int back  = analogRead(BACK_SENSOR);

  Serial.print("L: "); Serial.print(left);
  Serial.print(" R: "); Serial.print(right);
  Serial.print(" B: "); Serial.println(back);

  controlMotor(left, right, back);

  delay(5000);  // required sample time
}

void controlMotor(int left, int right, int back) {

  if (left > right && left > back) {

    Serial.println("Motor: CCW (left brightest)");

    analogWrite(9, 255);
    delay(1000);
    analogWrite(9, 0);

  }
  else if (right > left && right > back) {

    Serial.println("Motor: CW (right brightest)");

    analogWrite(9, 255);
    delay(1000);
    analogWrite(9, 0);

  }
  else if (back > left && back > right) {

    Serial.println("Motor: STOP (back brightest)");

    analogWrite(9, 0);
  }
}