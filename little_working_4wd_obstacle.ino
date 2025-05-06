#include <Arduino.h>
#include <ESP32Servo.h>

// Pin Definitions
const int rightMotorPin1 = 14;
const int rightMotorPin2 = 27;
const int leftMotorPin1  = 26;
const int leftMotorPin2  = 25;
const int sleepPin       = 33; // DRV8833 SLEEP (use a valid GPIO pin)
const int trigPin        = 12;
const int echoPin        = 13;
const int servoPin       = 15;

// PWM Channels
const int rightMotorPin1PWMChannel = 0;
const int rightMotorPin2PWMChannel = 1;
const int leftMotorPin1PWMChannel  = 2;
const int leftMotorPin2PWMChannel  = 3;

// PWM Settings
const int PWMFreq = 2000; // Hz
const int PWMResolution = 8; // 8 bits: 0-255

// Motor Speed
const int MAX_MOTOR_SPEED = 255;
const int BASE_SPEED = 127; // 50% speed

// Servo Scan Angles
const int SERVO_CENTER = 90;
const int SERVO_LEFT   = 150; // ~60° left from center
const int SERVO_RIGHT  = 30;  // ~60° right from center

// Obstacle Detection
const int OBSTACLE_DISTANCE_CM = 25;
const int DISTANCE_SAMPLES = 3; // For averaging

// Timing
const unsigned long REVERSE_TIME_MS = 2000;
const unsigned long REVERSE_EXTRA_MS = 1000;

// Globals
Servo scanServo;

// --- Function Prototypes ---
void setUpPinModes();
void rotateMotor(int rightMotorSpeed, int leftMotorSpeed);
void moveForward(int speed);
void moveBackward(int speed);
void turnLeft(int speed);
void turnRight(int speed);
void stopCar();
long measureDistance();
long scanLeftDistance();
long scanRightDistance();
long averageDistance(int samples);

// --- Setup ---
void setup() {
  setUpPinModes();
  scanServo.attach(servoPin);
  scanServo.write(SERVO_CENTER);
  delay(300); // Allow servo to center
  digitalWrite(sleepPin, HIGH); // Enable DRV8833
}

// --- Main Loop ---
void loop() {
  moveForward(BASE_SPEED);

  // Check for obstacle ahead
  if (averageDistance(DISTANCE_SAMPLES) < OBSTACLE_DISTANCE_CM) {
    // Obstacle detected: reverse
    moveBackward(BASE_SPEED);
    unsigned long t0 = millis();
    while (millis() - t0 < REVERSE_TIME_MS) {
      // Keep reversing
      delay(10);
    }
    stopCar();

    // Scan left and right
    long leftDist  = scanLeftDistance();
    long rightDist = scanRightDistance();

    // Decide direction
    if (leftDist >= OBSTACLE_DISTANCE_CM || rightDist >= OBSTACLE_DISTANCE_CM) {
      if (leftDist > rightDist) {
        // Turn left
        turnLeft(BASE_SPEED);
      } else {
        // Turn right
        turnRight(BASE_SPEED);
      }
      delay(500); // Turn for a short time
      stopCar();
    } else {
      // Both sides blocked: reverse extra and rescan
      moveBackward(BASE_SPEED);
      unsigned long t1 = millis();
      while (millis() - t1 < REVERSE_EXTRA_MS) {
        delay(10);
      }
      stopCar();
      // Rescan will happen on next loop
    }
  }
}

// --- Pin and PWM Setup ---
void setUpPinModes() {
  pinMode(rightMotorPin1, OUTPUT);
  pinMode(rightMotorPin2, OUTPUT);
  pinMode(leftMotorPin1, OUTPUT);
  pinMode(leftMotorPin2, OUTPUT);
  pinMode(sleepPin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Setup PWM channels
  ledcSetup(rightMotorPin1PWMChannel, PWMFreq, PWMResolution);
  ledcSetup(rightMotorPin2PWMChannel, PWMFreq, PWMResolution);
  ledcSetup(leftMotorPin1PWMChannel,  PWMFreq, PWMResolution);
  ledcSetup(leftMotorPin2PWMChannel,  PWMFreq, PWMResolution);

  ledcAttachPin(rightMotorPin1, rightMotorPin1PWMChannel);
  ledcAttachPin(rightMotorPin2, rightMotorPin2PWMChannel);
  ledcAttachPin(leftMotorPin1,  leftMotorPin1PWMChannel);
  ledcAttachPin(leftMotorPin2,  leftMotorPin2PWMChannel);
}

// --- Motor Control Logic (DRV8833) ---
void rotateMotor(int rightMotorSpeed, int leftMotorSpeed)
{
  // Right Motor
  if (rightMotorSpeed < 0) {
    ledcWrite(rightMotorPin1PWMChannel, 0);
    ledcWrite(rightMotorPin2PWMChannel, abs(rightMotorSpeed));
  } else if (rightMotorSpeed > 0) {
    ledcWrite(rightMotorPin1PWMChannel, abs(rightMotorSpeed));
    ledcWrite(rightMotorPin2PWMChannel, 0);
  } else {
    ledcWrite(rightMotorPin1PWMChannel, 0);
    ledcWrite(rightMotorPin2PWMChannel, 0);
  }

  // Left Motor
  if (leftMotorSpeed < 0) {
    ledcWrite(leftMotorPin1PWMChannel, 0);
    ledcWrite(leftMotorPin2PWMChannel, abs(leftMotorSpeed));
  } else if (leftMotorSpeed > 0) {
    ledcWrite(leftMotorPin1PWMChannel, abs(leftMotorSpeed));
    ledcWrite(leftMotorPin2PWMChannel, 0);
  } else {
    ledcWrite(leftMotorPin1PWMChannel, 0);
    ledcWrite(leftMotorPin2PWMChannel, 0);
  }
}

// --- Movement Functions ---
void moveForward(int speed) {
  rotateMotor(speed, speed);
}

void moveBackward(int speed) {
  rotateMotor(-speed, -speed);
}

void turnLeft(int speed) {
  rotateMotor(-speed, speed);
}

void turnRight(int speed) {
  rotateMotor(speed, -speed);
}

void stopCar() {
  rotateMotor(0, 0);
}

// --- Ultrasonic Distance Measurement ---
long measureDistance() {
  // Send trig pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read echo pulse
  long duration = pulseIn(echoPin, HIGH, 25000); // 25ms timeout (~4m max)
  if (duration == 0) {
    Serial.println("Ultrasonic sensor timeout: No echo received");
    return 400; // Out of range
  }

  // Calculate distance (cm)
  long distance = duration / 58;
  return distance;
}

// --- Averaged Distance for Debouncing ---
long averageDistance(int samples) {
  long sum = 0;
  int valid = 0;
  for (int i = 0; i < samples; i++) {
    long d = measureDistance();
    if (d > 0 && d < 400) { // Valid range
      sum += d;
      valid++;
    }
    delay(10);
  }
  if (valid == 0) return 400;
  return sum / valid;
}

long scanLeftDistance() {
  scanServo.write(SERVO_LEFT);
  delay(350); // Allow servo to reach position
  long dist = averageDistance(DISTANCE_SAMPLES);
  scanServo.write(SERVO_CENTER);
  delay(200);
  return dist;
}

long scanRightDistance() {
  scanServo.write(SERVO_RIGHT);
  delay(350);
  long dist = averageDistance(DISTANCE_SAMPLES);
  scanServo.write(SERVO_CENTER);
  delay(200);
  return dist;
}
