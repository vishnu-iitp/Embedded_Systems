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

// Flag to check if Dabble is connected
bool dabbleConnected = false;

void rotateMotor(int rightMotorSpeed, int leftMotorSpeed)
{
  if (rightMotorSpeed < 0)
  {
    ledcWrite(rightMotorPin1PWMChannel, 0);
    ledcWrite(rightMotorPin2PWMChannel, abs(rightMotorSpeed));
  }
  else if (rightMotorSpeed > 0)
  {
    ledcWrite(rightMotorPin1PWMChannel, abs(rightMotorSpeed));
    ledcWrite(rightMotorPin2PWMChannel, 0);
  }
  else
  {
    ledcWrite(rightMotorPin1PWMChannel, 0);
    ledcWrite(rightMotorPin2PWMChannel, 0);
  }
  
  if (leftMotorSpeed < 0)
  {
    ledcWrite(leftMotorPin1PWMChannel, 0);
    ledcWrite(leftMotorPin2PWMChannel, abs(leftMotorSpeed));
  }
  else if (leftMotorSpeed > 0)
  {
    ledcWrite(leftMotorPin1PWMChannel, abs(leftMotorSpeed));
    ledcWrite(leftMotorPin2PWMChannel, 0);
  }
  else
  {
    ledcWrite(leftMotorPin1PWMChannel, 0);
    ledcWrite(leftMotorPin2PWMChannel, 0);
  }
}

void setUpPinModes()
{
  pinMode(rightMotorPin1, OUTPUT);
  pinMode(rightMotorPin2, OUTPUT);
  pinMode(leftMotorPin1, OUTPUT);
  pinMode(leftMotorPin2, OUTPUT);
  
  pinMode(sleepPin, OUTPUT);
  digitalWrite(sleepPin, HIGH);

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
  Dabble.processInput();

  // Wait until a Gamepad input is detected to mark as connected
  if (!dabbleConnected)
  {
    if (GamePad.isUpPressed() || GamePad.isDownPressed() || GamePad.isLeftPressed() || GamePad.isRightPressed())
    {
      dabbleConnected = true;
    }
    else
    {
      // Stay idle if not connected yet
      rotateMotor(0, 0);
      return;
    }
  }

  int rightMotorSpeed = 0;
  int leftMotorSpeed = 0;

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
