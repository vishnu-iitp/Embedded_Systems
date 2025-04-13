#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// Wi-Fi Access Point credentials
const char* ssid = "ESP32_Car";
const char* password = "12345678";

WebServer server(80);
Servo steeringServo;

// Pin Definitions
#define SERVO_PIN  5
#define MOTOR_IN1  19
#define MOTOR_IN2  18

// Steering
#define SERVO_CENTER  90
#define SERVO_MIN     30
#define SERVO_MAX     155
#define SERVO_STEP    10

// Timing
#define MOTOR_TIMEOUT 10000
#define SERVO_TIMEOUT 3000
#define BRAKE_DURATION 650

int targetAngle = SERVO_CENTER;
unsigned long lastCommandTime = 0;
unsigned long lastSteerTime = 0;
bool isMoving = false;
bool isMovingForward = true;
unsigned long brakeStartTime = 0;
bool isBraking = false;

// HTML Webpage
String htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Car Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    button {
      width: 100px; height: 50px; margin: 10px;
      font-size: 18px; border-radius: 10px;
    }
  </style>
</head>
<body style="text-align:center; font-family:sans-serif;">
  <h2>ESP32 Wi-Fi Car Control</h2>
  <div>
    <button onclick="sendCmd('F')">Forward</button><br>
    <button onclick="sendCmd('L')">Left</button>
    <button onclick="sendCmd('O')">Stop</button>
    <button onclick="sendCmd('R')">Right</button><br>
    <button onclick="sendCmd('B')">Backward</button>
  </div>
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
  Serial.println("WiFi AP Started");
  Serial.println(WiFi.softAPIP());

  // Motor pins
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  stopMotor();

  // Servo
  steeringServo.attach(SERVO_PIN);
  steeringServo.write(SERVO_CENTER);

  // Web routes
  server.on("/", []() { server.send(200, "text/html", htmlPage); });
  server.on("/F", []() { processCommand('F'); server.send(200, "text/plain", "Forward"); });
  server.on("/B", []() { processCommand('B'); server.send(200, "text/plain", "Backward"); });
  server.on("/L", []() { processCommand('L'); server.send(200, "text/plain", "Left"); });
  server.on("/R", []() { processCommand('R'); server.send(200, "text/plain", "Right"); });
  server.on("/O", []() { processCommand('O'); server.send(200, "text/plain", "Stop"); });
  server.begin();
}

void loop() {
  server.handleClient();
  updateSteering();
  enforceTimeouts();
  manageBraking();
}

// --- Functionality from original code ---
void processCommand(char cmd) {
  cmd = toupper(cmd);
  switch (cmd) {
    case 'F': moveMotor(true); break;
    case 'B': moveMotor(false); break;
    case 'R': targetAngle = constrain(targetAngle - SERVO_STEP, SERVO_MIN, SERVO_MAX); break;
    case 'L': targetAngle = constrain(targetAngle + SERVO_STEP, SERVO_MIN, SERVO_MAX); break;
    case 'O':
      if (isMoving) {
        startBraking();
      } else {
        stopMotor();
        targetAngle = SERVO_CENTER;
      }
      break;
  }
  lastCommandTime = millis();
  lastSteerTime = millis();
}

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
    if (isMovingForward) {
      digitalWrite(MOTOR_IN1, LOW);
      digitalWrite(MOTOR_IN2, HIGH);
      Serial.println("Braking from forward");
    } else {
      digitalWrite(MOTOR_IN1, HIGH);
      digitalWrite(MOTOR_IN2, LOW);
      Serial.println("Braking from backward");
    }
  }
}

void manageBraking() {
  if (isBraking && (millis() - brakeStartTime >= BRAKE_DURATION)) {
    stopMotor();
    targetAngle = SERVO_CENTER;
    Serial.println("Braking complete");
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
