#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <DNSServer.h>  // Added for captive portal

// Wi-Fi Access Point credentials
const char* ssid = "ESP32_Car";
const char* password = "12345678";

WebServer server(80);
Servo steeringServo;
DNSServer dnsServer;  // Added for captive portal
const byte DNS_PORT = 53;  // Standard DNS port

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
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  <style>
    body {
      margin: 0;
      padding: 0;
      font-family: 'Segoe UI', sans-serif;
      background: #111;
      color: white;
      overflow: hidden;
    }

    .container {
      display: grid;
      grid-template-areas:
        "forward stop right"
        "backward stop left";
      height: 100vh;
      width: 100vw;
      padding: 20px;
      box-sizing: border-box;
    }

    button {
      width: 90px;
      height: 70px;
      font-size: 18px;
      font-weight: bold;
      border: none;
      border-radius: 15px;
      background: linear-gradient(145deg, #2c2c2c, #1a1a1a);
      color: #00ffcc;
      box-shadow: 0 4px 15px rgba(0, 255, 204, 0.2), 0 0 10px rgba(0, 255, 204, 0.1);
      transition: transform 0.15s ease, box-shadow 0.3s ease, background 0.3s ease;
    }

    button:hover {
      box-shadow: 0 0 20px rgba(0, 255, 204, 0.5), 0 0 40px rgba(0, 255, 204, 0.2);
    }

    button:active {
      transform: scale(0.92);
      background: linear-gradient(145deg, #1a1a1a, #2c2c2c);
      box-shadow: 0 2px 10px rgba(0, 255, 204, 0.1);
    }

    /* Button positioning */
    .forward { grid-area: forward; justify-self: start; align-self: center; }
    .backward { grid-area: backward; justify-self: start; align-self: center; }
    .left { grid-area: left; justify-self: end; align-self: center; }
    .right { grid-area: right; justify-self: end; align-self: center; }
    .stop { grid-area: stop; justify-self: center; align-self: center; }

    @media (max-height: 400px) {
      button {
        width: 80px;
        height: 60px;
        font-size: 16px;
      }
    }
  </style>
</head>
<body>
<div class="container">
    <button class="forward" onclick="sendCmd('F')">Forward</button>
    <button class="backward" onclick="sendCmd('B')">Backward</button>
    <button class="stop" onclick="sendCmd('O')">Stop</button>
    <button class="left" onclick="sendCmd('L')">Left</button>
    <button class="right" onclick="sendCmd('R')">Right</button>
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

  // Start DNS server for captive portal
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  Serial.println("DNS Server started");

  // Motor pins
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  stopMotor();

  // Servo
  steeringServo.attach(SERVO_PIN);
  steeringServo.write(SERVO_CENTER);

  // Web routes
  server.on("/", handleRoot);
  server.on("/F", []() { processCommand('F'); server.send(200, "text/plain", "Forward"); });
  server.on("/B", []() { processCommand('B'); server.send(200, "text/plain", "Backward"); });
  server.on("/L", []() { processCommand('L'); server.send(200, "text/plain", "Left"); });
  server.on("/R", []() { processCommand('R'); server.send(200, "text/plain", "Right"); });
  server.on("/O", []() { processCommand('O'); server.send(200, "text/plain", "Stop"); });
  
  // Handler for captive portal - redirect all unknown requests to the car control interface
  server.onNotFound(handleNotFound);
  
  server.begin();
}

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleNotFound() {
  // For captive portal, redirect all requests to the root page
  server.sendHeader("Location", "http://" + WiFi.softAPIP().toString());
  server.send(302, "text/plain", "");  // 302 is a temporary redirect
}

void loop() {
  dnsServer.processNextRequest();  // Handle DNS requests
  server.handleClient();
  updateSteering();
  enforceTimeouts();
  manageBraking();
}

// --- Existing functionality from original code ---
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
