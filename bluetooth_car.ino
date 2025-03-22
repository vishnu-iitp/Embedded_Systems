#include <BluetoothSerial.h>
#include <ESP32Servo.h>

// Pin Definitions
#define SERVO_PIN  18  // Servo for steering
#define MOTOR_IN1  19  // Motor driver input 1
#define MOTOR_IN2  21  // Motor driver input 2

// Servo Steering Limits
#define SERVO_CENTER  90  // Neutral position
#define SERVO_MIN     30  // Leftmost
#define SERVO_MAX     155 // Rightmost
#define SERVO_STEP    10  // Turning step size

// Timeout Values
#define MOTOR_TIMEOUT 10000 // Stop motor after 10s inactivity
#define SERVO_TIMEOUT 3000  // Auto-center servo after 3s

BluetoothSerial SerialBT;
Servo steeringServo;

// Global Variables
int targetAngle = SERVO_CENTER;
unsigned long lastCommandTime = 0;
unsigned long lastSteerTime = 0;
bool isMoving = false;

void setup() {
    Serial.begin(115200);
    SerialBT.begin("ESP32_Car");

    // Motor Setup
    pinMode(MOTOR_IN1, OUTPUT);
    pinMode(MOTOR_IN2, OUTPUT);
    stopMotor(); // Ensure motor is off at startup

    // Servo Setup
    steeringServo.attach(SERVO_PIN);
    steeringServo.write(SERVO_CENTER);
}

void loop() {
    checkBluetooth();
    updateSteering();
    enforceTimeouts();
}

// Handle Bluetooth Commands
void checkBluetooth() {
    if (SerialBT.available()) {
        char command = SerialBT.read();
        processCommand(command);
        lastCommandTime = millis();
        lastSteerTime = millis();
    }
}

// Process Incoming Commands
void processCommand(char cmd) {
    switch (toupper(cmd)) {
        case 'F': moveMotor(true); break;   // Move forward
        case 'B': moveMotor(false); break;  // Move backward
        case 'L': targetAngle = constrain(targetAngle - SERVO_STEP, SERVO_MIN, SERVO_MAX); break;
        case 'R': targetAngle = constrain(targetAngle + SERVO_STEP, SERVO_MIN, SERVO_MAX); break;
        case 'O': stopMotor(); targetAngle = SERVO_CENTER; break; // Stop & reset steering
    }
}

// Motor Control Without PWM (Full Speed)
void moveMotor(bool forward) {
    if (forward) {
        digitalWrite(MOTOR_IN1, HIGH);
        digitalWrite(MOTOR_IN2, LOW);
    } else {
        digitalWrite(MOTOR_IN1, LOW);
        digitalWrite(MOTOR_IN2, HIGH);
    }
    isMoving = true;
}

// Stop the Motor
void stopMotor() {
    digitalWrite(MOTOR_IN1, LOW);
    digitalWrite(MOTOR_IN2, LOW);
    isMoving = false;
}

// Smoothly Update Steering
void updateSteering() {
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate >= 10) {  // Small delay for smooth movement
        static int currentAngle = SERVO_CENTER;
        if (currentAngle != targetAngle) {
            currentAngle += (targetAngle > currentAngle) ? SERVO_STEP : -SERVO_STEP;
            steeringServo.write(currentAngle);
        }
        lastUpdate = millis();
    }

    // Auto-center servo after timeout
    if (millis() - lastSteerTime > SERVO_TIMEOUT) {
        targetAngle = SERVO_CENTER;
    }
}

// Stop Motor If No Command for a While
void enforceTimeouts() {
    if (isMoving && millis() - lastCommandTime > MOTOR_TIMEOUT) {
        stopMotor();
    }
}
