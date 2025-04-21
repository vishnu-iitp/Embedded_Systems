#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE
#include <DabbleESP32.h>

// Right motor pins - no enable pin for DRV8833
int rightMotorPin1 = 14;
int rightMotorPin2 = 27;

// Left motor pins - no enable pin for DRV8833
int leftMotorPin1 = 26;
int leftMotorPin2 = 25;

// Optional: Sleep pin for DRV8833 - adjust pin number as needed
int sleepPin = 20;

#define MAX_MOTOR_SPEED 255

const int PWMFreq = 1000; /* 1 KHz */
const int PWMResolution = 8;
const int rightMotorPin1PWMChannel = 4;
const int rightMotorPin2PWMChannel = 5;
const int leftMotorPin1PWMChannel = 6;
const int leftMotorPin2PWMChannel = 7;

void rotateMotor(int rightMotorSpeed, int leftMotorSpeed)
{
  if (rightMotorSpeed < 0)
  {
    // Going backward: Pin1 LOW, Pin2 PWM
    ledcWrite(rightMotorPin1PWMChannel, 0);
    ledcWrite(rightMotorPin2PWMChannel, abs(rightMotorSpeed));
  }
  else if (rightMotorSpeed > 0)
  {
    // Going forward: Pin1 PWM, Pin2 LOW
    ledcWrite(rightMotorPin1PWMChannel, abs(rightMotorSpeed));
    ledcWrite(rightMotorPin2PWMChannel, 0);
  }
  else
  {
    // Stopped: Both pins LOW
    ledcWrite(rightMotorPin1PWMChannel, 0);
    ledcWrite(rightMotorPin2PWMChannel, 0);
  }
  
  if (leftMotorSpeed < 0)
  {
    // Going backward: Pin1 LOW, Pin2 PWM
    ledcWrite(leftMotorPin1PWMChannel, 0);
    ledcWrite(leftMotorPin2PWMChannel, abs(leftMotorSpeed));
  }
  else if (leftMotorSpeed > 0)
  {
    // Going forward: Pin1 PWM, Pin2 LOW
    ledcWrite(leftMotorPin1PWMChannel, abs(leftMotorSpeed));
    ledcWrite(leftMotorPin2PWMChannel, 0);
  }
  else
  {
    // Stopped: Both pins LOW
    ledcWrite(leftMotorPin1PWMChannel, 0);
    ledcWrite(leftMotorPin2PWMChannel, 0);
  }
}

void setUpPinModes()
{
  // Setup direction pins
  pinMode(rightMotorPin1, OUTPUT);
  pinMode(rightMotorPin2, OUTPUT);
  pinMode(leftMotorPin1, OUTPUT);
  pinMode(leftMotorPin2, OUTPUT);
  
  // Setup SLP pin to enable the DRV8833 (required on most modules)
  pinMode(sleepPin, OUTPUT);
  digitalWrite(sleepPin, HIGH);  // Enable the DRV8833

  // Set up PWM for all direction pins
  ledcSetup(rightMotorPin1PWMChannel, PWMFreq, PWMResolution);
  ledcSetup(rightMotorPin2PWMChannel, PWMFreq, PWMResolution);
  ledcSetup(leftMotorPin1PWMChannel, PWMFreq, PWMResolution);
  ledcSetup(leftMotorPin2PWMChannel, PWMFreq, PWMResolution);
  
  ledcAttachPin(rightMotorPin1, rightMotorPin1PWMChannel);
  ledcAttachPin(rightMotorPin2, rightMotorPin2PWMChannel);
  ledcAttachPin(leftMotorPin1, leftMotorPin1PWMChannel);
  ledcAttachPin(leftMotorPin2, leftMotorPin2PWMChannel);

  rotateMotor(0, 0);
}

void setup()
{
  setUpPinModes();
  Dabble.begin("MyBluetoothCar"); 
}

void loop()
{
  int rightMotorSpeed = 0;
  int leftMotorSpeed = 0;
  Dabble.processInput();
  if (GamePad.isUpPressed())
  {
    rightMotorSpeed = MAX_MOTOR_SPEED;
    leftMotorSpeed = MAX_MOTOR_SPEED;
  }

  if (GamePad.isDownPressed())
  {
    rightMotorSpeed = -MAX_MOTOR_SPEED;
    leftMotorSpeed = -MAX_MOTOR_SPEED;
  }

  if (GamePad.isLeftPressed())
  {
    rightMotorSpeed = MAX_MOTOR_SPEED;
    leftMotorSpeed = -MAX_MOTOR_SPEED;
  }

  if (GamePad.isRightPressed())
  {
    rightMotorSpeed = -MAX_MOTOR_SPEED;
    leftMotorSpeed = MAX_MOTOR_SPEED;
  }

  rotateMotor(rightMotorSpeed, leftMotorSpeed);
}
