#include <BluetoothSerial.h>
#include <ESP32Servo.h>

// Pin Definitions
#define SERVO_PIN  5  // Servo for steering
#define MOTOR_IN1  19  // Motor driver input 1
#define MOTOR_IN2  18  // Motor driver input 2

// Servo Steering Limits
#define SERVO_CENTER  90  // Neutral position
#define SERVO_MIN     30  // Leftmost
#define SERVO_MAX     155 // Rightmost
#define SERVO_STEP    10  // Turning step size

// Timeout Values
#define MOTOR_TIMEOUT 10000 // Stop motor after 10s inactivity
#define SERVO_TIMEOUT 3000  // Auto-center servo after 3s
#define BRAKE_DURATION 650 // Braking duration in milliseconds

BluetoothSerial SerialBT;
Servo steeringServo;

// Global Variables
int targetAngle = SERVO_CENTER;
unsigned long lastCommandTime = 0;
unsigned long lastSteerTime = 0;
bool isMoving = false;
bool isMovingForward = true;  // Track direction: true=forward, false=backward
unsigned long brakeStartTime = 0;
bool isBraking = false;

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
    manageBraking();  // New function to handle braking timing
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
        case 'R': targetAngle = constrain(targetAngle - SERVO_STEP, SERVO_MIN, SERVO_MAX); break;
        case 'L': targetAngle = constrain(targetAngle + SERVO_STEP, SERVO_MIN, SERVO_MAX); break;
        case 'O': 
            if (isMoving) {
                startBraking();  // Begin braking process
            } else {
                stopMotor();
                targetAngle = SERVO_CENTER;
            }
            break;
    }
}

// Start the braking process
void startBraking() {
    if (!isBraking && isMoving) {
        isBraking = true;
        brakeStartTime = millis();
        
        // Apply reverse power based on current direction
        if (isMovingForward) {
            // Was moving forward, apply reverse power
            digitalWrite(MOTOR_IN1, LOW);
            digitalWrite(MOTOR_IN2, HIGH);
            Serial.println("Braking from forward motion");
        } else {
            // Was moving backward, apply forward power
            digitalWrite(MOTOR_IN1, HIGH);
            digitalWrite(MOTOR_IN2, LOW);
            Serial.println("Braking from backward motion");
        }
    }
}

// Manage the braking timing
void manageBraking() {
    if (isBraking && (millis() - brakeStartTime >= BRAKE_DURATION)) {
        // Braking duration completed, now fully stop
        stopMotor();
        targetAngle = SERVO_CENTER;
        isBraking = false;
        Serial.println("Braking completed, motor stopped");
    }
}

// Motor Control Without PWM (Full Speed)
void moveMotor(bool forward) {
    if (forward) {
        digitalWrite(MOTOR_IN1, HIGH);
        digitalWrite(MOTOR_IN2, LOW);
        isMovingForward = true;
    } else {
        digitalWrite(MOTOR_IN1, LOW);
        digitalWrite(MOTOR_IN2, HIGH);
        isMovingForward = false;
    }
    isMoving = true;
    isBraking = false;  // Cancel any ongoing braking
}

// Stop the Motor
void stopMotor() {
    digitalWrite(MOTOR_IN1, LOW);
    digitalWrite(MOTOR_IN2, LOW);
    isMoving = false;
    isBraking = false;
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
    if (isMoving && !isBraking && millis() - lastCommandTime > MOTOR_TIMEOUT) {
        startBraking();  // Use braking instead of immediate stop
    }
}
