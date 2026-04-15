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
#define motorPin1 9
#define motorPin2 10
#define TX 2
#define RX 3

const float shakeTreshold = 5.0;
bool isShaken = false;

SoftwareSerial obcSerial(RX, TX);

void setup() {
  Serial.begin(9600);
  obcSerial.begin(9600);
  Serial.println("OBC connected");
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  if( !bno.begin()) {
    Serial.println("No BNO055 detected");
    while(1);
  }
  delay(1000);
}

void loop() {

  int left  = analogRead(LEFT_SENSOR);
  int right = analogRead(RIGHT_SENSOR);
  int back  = analogRead(BACK_SENSOR);

  if (obcSerial.available()) {
    String recieveData = obcSerial.readStringUntil('\n');
    recieveData.trim();

    Serial.print("Recieved from OBC: ");
    Serial.println(recieveData);

    if (recieveData == "SUN") {
      sendSunData(left, right, back);
    }
    if (recieveData == "ADJ") {
      controlMotor(left, right, back);
    }
    if (recieveData == "IMU") {
      sendIMUData();
    }
    if (recieveData == "TEMP") {
      sendTempData();
    }
    if (recieveData == "SHAKE") {
      shake();
    }
    else {
      obcSerial.println("ERROR UNKNOWN COMMAND");
    }
  }



  delay(5000);  // required sample time
}



void controlMotor(int left, int right, int back) {

  if (left > right && left > back) {

    obcSerial.println("Motor: CCW (left brightest)");

    analogWrite(motorPin1, 255);
    delay(1000);
    analogWrite(motorPin2, 0);

  }
  else if (right > left && right > back) {

    obcSerial.println("Motor: CW (right brightest)");

    analogWrite(motorPin2, 255);
    delay(1000);
    analogWrite(motorPin1, 0);

  }
  else if (back > left && back > right) {

    obcSerial.println("Motor: STOP (back brightest)");

    analogWrite(motorPin1, 0);
    analogWrite(motorPin2, 0);
  }
}

void sendIMUData() {
  sensors_event_t event;
  bno.getEvent(&event);

  //Accelrations
  /*Serial.print("AccX: "); Serial.print(event.acceleration.x);
  Serial.print("AccY: "); Serial.print(event.acceleration.y);
  Serial.print("AccZ: "); Serial.print(event.acceleration.z);*/

  //Euler angles degree
  imu::Vector<3> euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
  obcSerial.print(" | RotX: "); obcSerial.print(euler.x());
  obcSerial.print(" RotY: "); obcSerial.print(euler.y());
  obcSerial.print(" RotZ: "); obcSerial.println(euler.z());

  //gryoscope rad/s
  /*imu::Vector<3> gyro = bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);
  Serial.print(" | GyroX: "); Serial.print(gyro.x());
  Serial.print(" GyroY: "); Serial.print(gyro.y());
  Serial.print(" GyroZ: "); Serial.println(gyro.z());*/

  /*imu::Vector<3> accel = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
  Serial.print(" | AcelX: "); Serial.print(accel.x());
  Serial.print(" AcelY: "); Serial.print(accel.y());
  Serial.print(" AcelZ: "); Serial.println(accel.z());*/
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
        obcSerial.println("Shaken");

        //clockwise
        analogWrite(motorPin1, 255);
        analogWrite(motorPin2, 0);
        delay(1000);

        //stop motor
        analogWrite(motorPin1, 0);
        analogWrite(motorPin2, 0);
        delay(500);

        //counter clockwise
        analogWrite(motorPin1, 0);
        analogWrite(motorPin2, 255);
        delay(1000);

        //stop motor
        analogWrite(motorPin1, 0);
        analogWrite(motorPin2, 0);
        delay(500);

        obcSerial.println("Sequence complete, awaitn for next shake");
        Serial.println("shake signal send to obc");
      }
    }
}

void sendTempData() {
  int tempRaw = analogRead(TEMP_SENSOR);
  float tempVolt = tempRaw * (5.0/1023.0);
  float temperature = tempVolt * 100;
  obcSerial.println("Temperature ADC: " + String(temperature));
  Serial.println("Send temperature Data to obc");
}

void sendSunData(int left, int right, int back) {
  obcSerial.print("L: " + String(left));
  obcSerial.print(" | R: " + String(right));
  obcSerial.print(" | B: " + String(back));
}



