#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE
#include <DabbleESP32.h>
#include <ESP32Servo.h>

// ==== Motor Pins ====
#define IN1 14  // Right Motor +
#define IN2 27  // Right Motor -
#define IN3 26  // Left Motor +
#define IN4 25  // Left Motor -

// PWM channels
#define PWMFreq 1000
#define PWMResolution 8
#define CH_IN1 4
#define CH_IN2 5
#define CH_IN3 6
#define CH_IN4 7

// ==== Ultrasonic Pins ====
#define trigPin 5
#define echoPin 18

// ==== Servo ====
#define servoPin 15
Servo myServo;

// ==== Flags ====
bool obstacleAvoidanceMode = false;
bool dabbleConnected = false;

// ==== Obstacle Avoidance Variables ====
long duration;
int distance;
int rightDistance, leftDistance;

// ==== Constants ====
#define MAX_SPEED 255

void setupPWM() {
  ledcSetup(CH_IN1, PWMFreq, PWMResolution);
  ledcSetup(CH_IN2, PWMFreq, PWMResolution);
  ledcSetup(CH_IN3, PWMFreq, PWMResolution);
  ledcSetup(CH_IN4, PWMFreq, PWMResolution);

  ledcAttachPin(IN1, CH_IN1);
  ledcAttachPin(IN2, CH_IN2);
  ledcAttachPin(IN3, CH_IN3);
  ledcAttachPin(IN4, CH_IN4);
}

void stopCar() {
  ledcWrite(CH_IN1, 0);
  ledcWrite(CH_IN2, 0);
  ledcWrite(CH_IN3, 0);
  ledcWrite(CH_IN4, 0);
}

void moveForward() {
  ledcWrite(CH_IN1, MAX_SPEED); ledcWrite(CH_IN2, 0);
  ledcWrite(CH_IN3, MAX_SPEED); ledcWrite(CH_IN4, 0);
}

void moveBackward() {
  ledcWrite(CH_IN1, 0); ledcWrite(CH_IN2, MAX_SPEED);
  ledcWrite(CH_IN3, 0); ledcWrite(CH_IN4, MAX_SPEED);
}

void turnLeft() {
  ledcWrite(CH_IN1, 0); ledcWrite(CH_IN2, 150);
  ledcWrite(CH_IN3, 150); ledcWrite(CH_IN4, 0);
}

void turnRight() {
  ledcWrite(CH_IN1, 150); ledcWrite(CH_IN2, 0);
  ledcWrite(CH_IN3, 0); ledcWrite(CH_IN4, 150);
}

int getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;
  return distance;
}

void handleObstacleAvoidance() {
  myServo.write(90);
  delay(300);
  int front = getDistance();

  if (front < 20) {
    stopCar();
    delay(300);
    
    myServo.write(0);
    delay(300);
    leftDistance = getDistance();
    
    myServo.write(180);
    delay(300);
    rightDistance = getDistance();

    if (leftDistance > rightDistance) {
      turnLeft();
    } else {
      turnRight();
    }
    delay(600);
  } else {
    moveForward();
  }

  delay(100);
}

void handleManualControl() {
  int rightMotorSpeed = 0;
  int leftMotorSpeed = 0;

  if (GamePad.isUpPressed()) {
    rightMotorSpeed = MAX_SPEED;
    leftMotorSpeed = MAX_SPEED;
  }

  if (GamePad.isDownPressed()) {
    rightMotorSpeed = -MAX_SPEED;
    leftMotorSpeed = -MAX_SPEED;
  }

  if (GamePad.isLeftPressed()) {
    rightMotorSpeed = MAX_SPEED;
    leftMotorSpeed = -MAX_SPEED;
  }

  if (GamePad.isRightPressed()) {
    rightMotorSpeed = -MAX_SPEED;
    leftMotorSpeed = MAX_SPEED;
  }

  // Drive Motors
  if (rightMotorSpeed < 0) {
    ledcWrite(CH_IN1, 0);
    ledcWrite(CH_IN2, abs(rightMotorSpeed));
  } else {
    ledcWrite(CH_IN1, abs(rightMotorSpeed));
    ledcWrite(CH_IN2, 0);
  }

  if (leftMotorSpeed < 0) {
    ledcWrite(CH_IN3, 0);
    ledcWrite(CH_IN4, abs(leftMotorSpeed));
  } else {
    ledcWrite(CH_IN3, abs(leftMotorSpeed));
    ledcWrite(CH_IN4, 0);
  }
}

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  setupPWM();
  stopCar();

  myServo.attach(servoPin);
  myServo.write(90);

  Serial.begin(9600);
  Dabble.begin("SmartCar");
}

void loop() {
  Dabble.processInput();

  if (GamePad.isStartPressed()) {
    obstacleAvoidanceMode = true;
  }

  if (GamePad.isSelectPressed()) {
    obstacleAvoidanceMode = false;
    stopCar();
  }

  if (obstacleAvoidanceMode) {
    handleObstacleAvoidance();
  } else {
    if (!dabbleConnected && (GamePad.isUpPressed() || GamePad.isDownPressed() || GamePad.isLeftPressed() || GamePad.isRightPressed())) {
      dabbleConnected = true;
    }

    if (dabbleConnected) {
      handleManualControl();
    } else {
      stopCar();
    }
  }
}
