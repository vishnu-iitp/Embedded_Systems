#include <ESP32Servo.h>

// Pin Definitions
#define trigPin 33
#define echoPin 32
/*#define ENA 14
#define ENB 13*/
#define IN1 14
#define IN2 27
#define IN3 26
#define IN4 25
#define servoPin 13

Servo myServo;

// Variables for distance measurement
long duration;
int distance;
int rightDistance, leftDistance;

// Function to calculate distance
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

// Motor control functions
void moveForward() {
  analogWrite(IN1, 250); digitalWrite(IN2, LOW);  // Left motor forward
  analogWrite(IN3, 250); digitalWrite(IN4, LOW);  // Right motor forward
}

void moveBackward() {
  digitalWrite(IN1, LOW); analogWrite(IN2, 250);  // Left motor backward
  digitalWrite(IN3, LOW); analogWrite(IN4, 250);  // Right motor backward
}

void stopCar() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}

void turnLeft() {
  digitalWrite(IN1, LOW); analogWrite(IN2, 150);  // Left motor backward
  analogWrite(IN3, 150); digitalWrite(IN4, LOW);  // Right motor forward
}

void turnRight() {
  analogWrite(IN1, 150); digitalWrite(IN2, LOW);  // Left motor forward
  digitalWrite(IN3, LOW); analogWrite(IN4, 150);  // Right motor backward
}


void setup() {
  // Setup pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  

  // Servo setup
  myServo.attach(servoPin);
  myServo.write(90); // Start servo at center position

  Serial.begin(9600);
}

void loop() {
  // Scan forward
  myServo.write(90); // Center position (looking straight ahead)
  delay(500);
  int frontDistance = getDistance();

  // If an obstacle is closer than 20 cm
  if (frontDistance < 20) { 
    stopCar();
    delay(1000); // Pause to make a decision

    // Scan left side
    myServo.write(0); // Turn servo to the left
    delay(500);
    leftDistance = getDistance();

    // Scan right side
    myServo.write(180); // Turn servo to the right
    delay(500);
    rightDistance = getDistance();

    // Decide direction based on the distances
    if (leftDistance > rightDistance) {
      turnLeft();
      delay(1000); // Adjust the delay to control turning duration
    } else {
      turnRight();
      delay(1000); // Adjust the delay to control turning duration
    }

    stopCar(); // Stop after turning
    delay(1000); // Brief pause before moving forward again
  } else {
    moveForward(); // Continue moving forward if the path is clear
  }

  delay(200); // Short delay before next loop iteration
}
