#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

// Wi-Fi Access Point credentials
const char* ssid = "ESP32_Car";
const char* password = "12345678";

WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;  // Standard DNS port

// Motor pin definitions — change these to match your wiring:
#define LEFT_IN1 14
#define LEFT_IN2 27
#define RIGHT_IN1 26
#define RIGHT_IN2 25

// Timing
#define MOTOR_TIMEOUT 10000  // ms before auto-brake
#define BRAKE_DURATION 500   // updated to 500 milliseconds (changed)  // ms of reversing for brake

unsigned long lastCommandTime = 0;
unsigned long brakeStartTime = 0;
bool isMoving = false;
bool isBraking = false;
bool movingForward = true;

// HTML Webpage (buttons for F, B, L, R, O)
const char* htmlPage = R"rawliteral(
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
  <button class="forward"  onclick="sendCmd('F')">Forward</button>
  <button class="backward" onclick="sendCmd('B')">Backward</button>
  <button class="stop"     onclick="sendCmd('O')">Stop</button>
  <button class="left"     onclick="sendCmd('L')">Left</button>
  <button class="right"    onclick="sendCmd('R')">Right</button>
</div>
<script>
  function sendCmd(cmd) { fetch('/' + cmd); }
</script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  // start AP + captive portal
  WiFi.softAP(ssid, password);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  server.onNotFound(handleNotFound);

  // set up motor pins
  pinMode(LEFT_IN1, OUTPUT);
  pinMode(LEFT_IN2, OUTPUT);
  pinMode(RIGHT_IN1, OUTPUT);
  pinMode(RIGHT_IN2, OUTPUT);
  stopMotors();

  // web routes
  server.on("/", handleRoot);
  server.on("/F", []() {
    processCmd('F');
    server.send(200, "text/plain", "Forward");
  });
  server.on("/B", []() {
    processCmd('B');
    server.send(200, "text/plain", "Backward");
  });
  server.on("/L", []() {
    processCmd('L');
    server.send(200, "text/plain", "Left");
  });
  server.on("/R", []() {
    processCmd('R');
    server.send(200, "text/plain", "Right");
  });
  server.on("/O", []() {
    processCmd('O');
    server.send(000, "text/plain", "Stop");
  });
  server.begin();
  Serial.println("Setup complete. AP IP: " + WiFi.softAPIP().toString());
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  enforceTimeouts();
  manageBraking();
}

// ——— Command processing ——————————————————————————————————————————————————

void processCmd(char cmd) {
  cmd = toupper(cmd);
  lastCommandTime = millis();

  switch (cmd) {
    case 'F':  // both forward
      moveMotors(true, true);
      movingForward = true;
      break;
    case 'B':  // both backward
      moveMotors(false, false);
      movingForward = false;
      break;
    case 'L':  // spin left: left backward, right forward
      moveMotors(false, true);
      movingForward = false;
      break;
    case 'R':  // spin right: left forward, right backward
      moveMotors(true, false);
      movingForward = true;
      break;
    case 'O':  // Stop - brake by reversing briefly, then fully stop
        digitalWrite(LEFT_IN1, LOW);
        digitalWrite(LEFT_IN2, LOW);  // Motor A reverse
        digitalWrite(RIGHT_IN1, LOW);
        digitalWrite(RIGHT_IN2, LOW);  // Motor B reverse
        /*delay(BRAKE_DURATION);
        digitalWrite(LEFT_IN1, LOW);
        digitalWrite(LEFT_IN2, LOW);
        digitalWrite(RIGHT_IN3, LOW);
        digitalWrite(RIGHT_IN4, LOW);*/
        break;
  }
}

// drive logic:
//  leftFwd  = true  → left motor forward
//  leftFwd  = false → left motor backward
//  rightFwd = true  → right motor forward
//  rightFwd = false → right motor backward
void moveMotors(bool leftFwd, bool rightFwd) {
  // left motor
  digitalWrite(LEFT_IN1, leftFwd ? HIGH : LOW);
  digitalWrite(LEFT_IN2, leftFwd ? LOW : HIGH);
  // right motor
  digitalWrite(RIGHT_IN1, rightFwd ? HIGH : LOW);
  digitalWrite(RIGHT_IN2, rightFwd ? LOW : HIGH);

  isMoving = true;
  isBraking = false;
}

// full stop (both inputs LOW)
void stopMotors() {
  digitalWrite(LEFT_IN1, LOW);
  digitalWrite(LEFT_IN2, LOW);
  digitalWrite(RIGHT_IN1, LOW);
  digitalWrite(RIGHT_IN2, LOW);
  isMoving = false;
  isBraking = true;
}

// initiate braking by reversing for a short duration
void startBraking() {
  isBraking = true;
  brakeStartTime = millis();
  // reverse direction for brake
  moveMotors(!movingForward, !movingForward);
}

// after BRAKE_DURATION, stop fully
void manageBraking() {
  if (isBraking && (millis() - brakeStartTime >= BRAKE_DURATION)) {
    stopMotors();
  }
}

// auto-brake if no command in MOTOR_TIMEOUT
void enforceTimeouts() {
  if (isMoving && !isBraking && millis() - lastCommandTime > MOTOR_TIMEOUT) {
    startBraking();
  }
}

// serve main page
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

// captive portal redirect
void handleNotFound() {
  server.sendHeader("Location", "http://" + WiFi.softAPIP().toString());
  server.send(302, "text/plain", "");
}
