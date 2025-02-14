#include <BluetoothSerial.h>
#include <ESP32Servo.h>

BluetoothSerial SerialBT;
Servo steeringServo;

const int motorPin1 = 18;
const int motorPin2 = 19;
const int servoPin = 5;

int motorSpeed = 0; // 0% to 100%
int steeringAngle = 90; // Center position

void setup() {
    Serial.begin(115200);
    SerialBT.begin("ESP32_Car"); // Bluetooth device name

    pinMode(motorPin1, OUTPUT);
    pinMode(motorPin2, OUTPUT);

    steeringServo.attach(servoPin);
    steeringServo.write(steeringAngle); // Center position
}

void setMotorSpeed() {
    int pwmValue = (motorSpeed * 255) / 100; // Convert percentage to PWM value
    analogWrite(motorPin1, pwmValue);
    digitalWrite(motorPin2, motorSpeed > 0 ? LOW : HIGH);
}

void loop() {
    if (SerialBT.available()) {
        char command = SerialBT.read();
        Serial.println(command);

        switch (command) {
            case 'F': // Increase forward speed
                if (motorSpeed < 100) motorSpeed += 25;
                setMotorSpeed();
                break;
            case 'B': // Increase backward speed
                if (motorSpeed > -100) motorSpeed -= 25;
                setMotorSpeed();
                break;
            case 'L': // Turn left gradually
                if (steeringAngle > 60) steeringAngle -= 10;
                steeringServo.write(steeringAngle);
                break;
            case 'R': // Turn right gradually
                if (steeringAngle < 120) steeringAngle += 10;
                steeringServo.write(steeringAngle);
                break;
            case 'O': // Reduce speed gradually
                if (motorSpeed > 0) motorSpeed -= 25;
                else if (motorSpeed < 0) motorSpeed += 25;
                setMotorSpeed();
                break;
            case 'S': // Stop completely
                motorSpeed = 0;
                setMotorSpeed();
                steeringAngle = 90;
                steeringServo.write(steeringAngle);
                break;
        }
    }
}
