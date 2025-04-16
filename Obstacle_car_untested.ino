#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <DNSServer.h>

// WiFi AP credentials
const char* ssid = "ESP32_Car";
const char* password = "12345678";

WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

// Pins
#define MOTOR_IN1 19
#define MOTOR_IN2 18
#define SERVO_CAR_PIN 5
#define SERVO_SENSOR_PIN 4
#define TRIG_PIN 13
#define ECHO_PIN 12

// Constants
#define BRAKE_DURATION 650
#define MOTOR_TIMEOUT 10000
#define SERVO_TIMEOUT 3000
#define SERVO_CENTER 90
#define SERVO_MIN 30
#define SERVO_MAX 155
#define SERVO_STEP 10

Servo steeringServo;
Servo sensorServo;

unsigned long lastCommandTime = 0;
unsigned long lastSteerTime = 0;
unsigned long brakeStartTime = 0;

int targetAngle = SERVO_CENTER;
bool isMoving = false;
bool isMovingForward = true;
bool isBraking = false;
bool obstacleAvoidance = false;

String htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Car</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: 'Segoe UI'; background: #111; color: white; display: flex; flex-direction: column; align-items: center; padding: 20px; }
    button {
      margin: 10px; padding: 15px 30px;
      font-size: 18px; font-weight: bold;
      border: none; border-radius: 10px;
      background: #222; color: #00ffcc;
      box-shadow: 0 4px 10px rgba(0,255,204,0.3);
    }
    button:hover { background: #333; }
  </style>
</head>
<body>
  <button onclick="sendCmd('F')">Forward</button>
  <button onclick="sendCmd('B')">Backward</button>
  <button onclick="sendCmd('L')">Left</button>
  <button onclick="sendCmd('R')">Right</button>
  <button onclick="sendCmd('O')">Stop</button>
  <button onclick="sendCmd('T')">Toggle Obstacle Avoidance</button>
  <script>
    function sendCmd(cmd) {
      fetch('/' + cmd);
    }
  </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  server.begin();

  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  stopMotor();

  steeringServo.attach(SERVO_CAR_PIN);
  sensorServo.attach(SERVO_SENSOR_PIN);
  steeringServo.write(SERVO_CENTER);
  sensorServo.write(90);  // center forward

  server.on("/", []() { server.send(200, "text/html", htmlPage); });
  server.on("/F", []() { processCommand('F'); server.send(200); });
  server.on("/B", []() { processCommand('B'); server.send(200); });
  server.on("/L", []() { processCommand('L'); server.send(200); });
  server.on("/R", []() { processCommand('R'); server.send(200); });
  server.on("/O", []() { processCommand('O'); server.send(200); });
  server.on("/T", []() { obstacleAvoidance = !obstacleAvoidance; server.send(200); });
  server.onNotFound([]() {
    server.sendHeader("Location", "http://" + WiFi.softAPIP().toString());
    server.send(302, "text/plain", "");
  });
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  updateSteering();
  enforceTimeouts();
  manageBraking();

  if (obstacleAvoidance) runObstacleAvoidance();
}

void processCommand(char cmd) {
  cmd = toupper(cmd);
  switch (cmd) {
    case 'F': moveMotor(true); break;
    case 'B': moveMotor(false); break;
    case 'L': targetAngle = constrain(targetAngle + SERVO_STEP, SERVO_MIN, SERVO_MAX); break;
    case 'R': targetAngle = constrain(targetAngle - SERVO_STEP, SERVO_MIN, SERVO_MAX); break;
    case 'O': stopMotor(); targetAngle = SERVO_CENTER; break;
  }
  lastCommandTime = millis();
  lastSteerTime = millis();
}

void moveMotor(bool forward) {
  digitalWrite(MOTOR_IN1, forward ? HIGH : LOW);
  digitalWrite(MOTOR_IN2, forward ? LOW : HIGH);
  isMoving = true;
  isMovingForward = forward;
  isBraking = false;
}

void stopMotor() {
  digitalWrite(MOTOR_IN1, LOW);
  digitalWrite(MOTOR_IN2, LOW);
  isMoving = false;
  isBraking = false;
}

void startBraking() {
  if (!isBraking && isMoving) {
    isBraking = true;
    brakeStartTime = millis();
    digitalWrite(MOTOR_IN1, isMovingForward ? LOW : HIGH);
    digitalWrite(MOTOR_IN2, isMovingForward ? HIGH : LOW);
  }
}

void manageBraking() {
  if (isBraking && millis() - brakeStartTime >= BRAKE_DURATION) {
    stopMotor();
    targetAngle = SERVO_CENTER;
  }
}

void updateSteering() {
  static unsigned long lastUpdate = 0;
  static int currentAngle = SERVO_CENTER;
  if (millis() - lastUpdate >= 10) {
    if (currentAngle != targetAngle) {
      currentAngle += (targetAngle > currentAngle) ? SERVO_STEP : -SERVO_STEP;
      steeringServo.write(currentAngle);
    }
    lastUpdate = millis();
  }

  if (millis() - lastSteerTime > SERVO_TIMEOUT) {
    targetAngle = SERVO_CENTER;
  }
}

void enforceTimeouts() {
  if (isMoving && !isBraking && millis() - lastCommandTime > MOTOR_TIMEOUT) {
    startBraking();
  }
}

float getDistanceCM() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  return (duration > 0) ? (duration * 0.0343) / 2.0 : 999;
}

void runObstacleAvoidance() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < 500) return;
  lastCheck = millis();

  float front = readDir(90);
  if (front < 20) {
    stopMotor();
    delay(300);
    moveMotor(false); delay(500); stopMotor();
    float left = readDir(160);
    float right = readDir(20);

    if (left > 25) {
      Serial.println("Turning Left");
      targetAngle = SERVO_MAX;
      moveMotor(true);
      delay(700);
    } else if (right > 25) {
      Serial.println("Turning Right");
      targetAngle = SERVO_MIN;
      moveMotor(true);
      delay(700);
    } else {
      Serial.println("Only obstacle front, go left");
      targetAngle = SERVO_MAX;
      moveMotor(true);
      delay(700);
    }
    stopMotor();
    targetAngle = SERVO_CENTER;
  } else {
    moveMotor(true);
  }
}

float readDir(int angle) {
  sensorServo.write(angle);
  delay(400);
  float dist = getDistanceCM();
  delay(100);
  return dist;
}
