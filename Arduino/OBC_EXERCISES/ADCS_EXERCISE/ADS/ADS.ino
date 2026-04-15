#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

Adafruit_BNO055 bno = Adafruit_BNO055(55);

#define RIGHT_SENSOR A0
#define BACK_SENSOR  A2 
#define LEFT_SENSOR  A3 
int motorPin1 = 9;
int motorPin2 = 10;

const float shakeTreshold = 5.0;
bool isShaken = false;

void setup() {
  Serial.begin(9600);
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  if( !bno.begin()) {
    Serial.println("No BNO055 detected");
    while(1);
  }
  delay(1000);
}

void loop() {

  realAllIMUData();
  shake();
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

    analogWrite(motorPin1, 255);
    delay(1000);
    analogWrite(motorPin2, 0);

  }
  else if (right > left && right > back) {

    Serial.println("Motor: CW (right brightest)");

    analogWrite(motorPin2, 255);
    delay(1000);
    analogWrite(motorPin1, 0);

  }
  else if (back > left && back > right) {

    Serial.println("Motor: STOP (back brightest)");

    analogWrite(motorPin1, 0);
    analogWrite(motorPin2, 0);
  }
}

void shake() {
  imu::Vector<3> accel = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);

  sensors_event_t event;
  bno.getEvent(&event);

  //if accelration exceed trashhold
  if(abs(accel.x()) > (shakeTreshold) ||
    abs(accel.y()) > shakeTreshold ||
    abs(accel.z()) > shakeTreshold) {
      if (!isShaken) {
        Serial.println("Shaken");

        //clockwise
        analogWrite(motorPin1, 255);
        analogWrite(motorPin2, 0);
        delay(500);

        //stop motor
        analogWrite(motorPin1, 0);
        analogWrite(motorPin2, 0);
        delay(100);

        //counter clockwise
        analogWrite(motorPin1, 0);
        analogWrite(motorPin2, 255);
        delay(500);

        //stop motor
        analogWrite(motorPin1, 0);
        analogWrite(motorPin2, 0);
        delay(100);

        Serial.println("Sequence complete, awaitn for next shake");
      }
    }
}

void realAllIMUData() {
  sensors_event_t event;
  bno.getEvent(&event);

  //Accelrations
  /*Serial.print("AccX: "); Serial.print(event.acceleration.x);
  Serial.print("AccY: "); Serial.print(event.acceleration.y);
  Serial.print("AccZ: "); Serial.print(event.acceleration.z);*/

  //Euler angles degree
  /*imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
  Serial.print(" | RotX: "); Serial.print(euler.x());
  Serial.print(" RotY: "); Serial.print(euler.y());
  Serial.print(" RotZ: "); Serial.println(euler.z());*/

  //gryoscope rad/s
  /*imu::Vector<3> gyro = bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);
  Serial.print(" | GyroX: "); Serial.print(gyro.x());
  Serial.print(" GyroY: "); Serial.print(gyro.y());
  Serial.print(" GyroZ: "); Serial.println(gyro.z());*/

  imu::Vector<3> accel = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
  Serial.print(" | AcelX: "); Serial.print(accel.x());
  Serial.print(" AcelY: "); Serial.print(accel.y());
  Serial.print(" AcelZ: "); Serial.println(accel.z());
}

