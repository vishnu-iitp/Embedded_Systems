#include <ESP32Servo.h>

// Pin Definitions
const int AIN1 = 14;
const int AIN2 = 27;
const int BIN1 = 26;
const int BIN2 = 25;
const int SERVO_PIN = 16;
const int TRIG_PIN = 17;
const int ECHO_PIN = 18;

// PWM Settings
const int PWM_FREQ = 5000;
const int PWM_RESOLUTION = 8;
const int PWM_HALF_SPEED = 127;  // 50% duty cycle

// Motor PWM Channels
const int CH_AIN1 = 0;
const int CH_AIN2 = 1;
const int CH_BIN1 = 2;
const int CH_BIN2 = 3;

// Constants
const int OBSTACLE_DISTANCE = 20;  // cm
const int TURN_DURATION = 500;     // ms
const int SCAN_DELAY = 500;        // ms

Servo servo;

void setup() {
  Serial.begin(115200);
  
  // Motor Driver Setup
  ledcSetup(CH_AIN1, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(AIN1, CH_AIN1);
  ledcSetup(CH_AIN2, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(AIN2, CH_AIN2);
  ledcSetup(CH_BIN1, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(BIN1, CH_BIN1);
  ledcSetup(CH_BIN2, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(BIN2, CH_BIN2);

  // Servo Setup
  servo.attach(SERVO_PIN);
  centerServo();
  
  // Ultrasonic Setup
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void loop() {
  moveForward();
  
  // Obstacle Detection Loop
  while(true) {
    int distance = getDistance();
    if(distance > 0 && distance <= OBSTACLE_DISTANCE) {
      handleObstacle();
      break;
    }
    delay(100);
  }
}

void handleObstacle() {
  stopMotors();
  avoidObstacle();
  scanEnvironment();
}

void avoidObstacle() {
  moveBackward();
  delay(2000);
  stopMotors();
}

void scanEnvironment() {
  int distances[3];
  
  // Scan Left
  servo.write(0);
  delay(SCAN_DELAY);
  distances[0] = getDistance();
  
  // Scan Center
  servo.write(90);
  delay(SCAN_DELAY);
  distances[1] = getDistance();
  
  // Scan Right
  servo.write(180);
  delay(SCAN_DELAY);
  distances[2] = getDistance();
  
  centerServo();
  decideMovement(distances);
}

void decideMovement(int distances[]) {
  bool leftClear = distances[0] > OBSTACLE_DISTANCE || distances[0] == -1;
  bool centerClear = distances[1] > OBSTACLE_DISTANCE || distances[1] == -1;
  bool rightClear = distances[2] > OBSTACLE_DISTANCE || distances[2] == -1;

  if(centerClear) {
    moveForward();
  }
  else if(leftClear && rightClear) {
    turnLeft();
    delay(TURN_DURATION);
    moveForward();
  }
  else if(leftClear) {
    turnLeft();
    delay(TURN_DURATION);
    moveForward();
  }
  else if(rightClear) {
    turnRight();
    delay(TURN_DURATION);
    moveForward();
  }
  else {
    moveBackward();
    delay(4000);
    stopMotors();
    scanEnvironment();
  }
}

int getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if(duration == 0) return -1;
  
  int distance = duration * 0.034 / 2;
  return (distance > 400) ? -1 : distance;  // Limit to 4 meters
}

// Motor Control Functions
void moveForward() {
  ledcWrite(CH_AIN1, PWM_HALF_SPEED);
  ledcWrite(CH_AIN2, 0);
  ledcWrite(CH_BIN1, PWM_HALF_SPEED);
  ledcWrite(CH_BIN2, 0);
}

void moveBackward() {
  ledcWrite(CH_AIN1, 0);
  ledcWrite(CH_AIN2, PWM_HALF_SPEED);
  ledcWrite(CH_BIN1, 0);
  ledcWrite(CH_BIN2, PWM_HALF_SPEED);
}

void turnLeft() {
  ledcWrite(CH_AIN1, 0);
  ledcWrite(CH_AIN2, PWM_HALF_SPEED);
  ledcWrite(CH_BIN1, PWM_HALF_SPEED);
  ledcWrite(CH_BIN2, 0);
}

void turnRight() {
  ledcWrite(CH_AIN1, PWM_HALF_SPEED);
  ledcWrite(CH_AIN2, 0);
  ledcWrite(CH_BIN1, 0);
  ledcWrite(CH_BIN2, PWM_HALF_SPEED);
}

void stopMotors() {
  ledcWrite(CH_AIN1, 0);
  ledcWrite(CH_AIN2, 0);
  ledcWrite(CH_BIN1, 0);
  ledcWrite(CH_BIN2, 0);
}

void centerServo() {
  servo.write(90);
  delay(SCAN_DELAY);
}
