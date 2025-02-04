/*
Wiring Setup
NEO-6M GPS Module → ESP32
GPS Module	ESP32 Pin
VCC	3.3V
GND	GND
TX	GPIO 16 (RX2)
RX	GPIO 17 (TX2)

ILI9341 TFT Display → ESP32
TFT Display	ESP32 Pin
VCC	3.3V
GND	GND
CS	GPIO 5
RST	GPIO 4
DC	GPIO 2
MOSI	GPIO 23
SCK	GPIO 18
LED	3.3V (Optional: Use 220Ω resistor)
MISO	GPIO 19 (Optional, can be left unconnected)*/
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

// Define pins for ILI9341 display
#define TFT_CS     5
#define TFT_RST    4
#define TFT_DC     2

// Initialize ILI9341 display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Initialize TinyGPS++ library
TinyGPSPlus gps;

// Initialize hardware serial port for GPS
HardwareSerial ss(2);

void setup() {
  Serial.begin(115200);  // Debugging output
  ss.begin(9600, SERIAL_8N1, 16, 17);  // GPS Module (TX = GPIO 16, RX = GPIO 17)

  // Initialize TFT display
  tft.begin();
  tft.setRotation(3);  // Adjust rotation as needed
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);

  tft.setCursor(20, 20);
  tft.print("GPS Tracker Initializing...");
  delay(2000);
}

void loop() {
  while (ss.available() > 0) {
    gps.encode(ss.read());
  }

  if (gps.location.isUpdated()) {
    tft.fillScreen(ILI9341_BLACK);  // Clear screen

    // Display Latitude
    tft.setCursor(20, 20);
    tft.setTextColor(ILI9341_CYAN);
    tft.print("Lat: ");
    tft.setTextColor(ILI9341_WHITE);
    tft.println(gps.location.lat(), 6);

    // Display Longitude
    tft.setCursor(20, 50);
    tft.setTextColor(ILI9341_CYAN);
    tft.print("Lon: ");
    tft.setTextColor(ILI9341_WHITE);
    tft.println(gps.location.lng(), 6);

    // Display Altitude
    tft.setCursor(20, 80);
    tft.setTextColor(ILI9341_CYAN);
    tft.print("Alt: ");
    tft.setTextColor(ILI9341_WHITE);
    tft.print(gps.altitude.meters());
    tft.println(" m");

    // Display Speed
    tft.setCursor(20, 110);
    tft.setTextColor(ILI9341_CYAN);
    tft.print("Speed: ");
    tft.setTextColor(ILI9341_WHITE);
    tft.print(gps.speed.kmph());
    tft.println(" km/h");

    // Display Number of Satellites
    tft.setCursor(20, 140);
    tft.setTextColor(ILI9341_CYAN);
    tft.print("Satellites: ");
    tft.setTextColor(ILI9341_WHITE);
    tft.println(gps.satellites.value());

    // Display Date and Time
    tft.setCursor(20, 170);
    tft.setTextColor(ILI9341_CYAN);
    if (gps.date.isValid() && gps.time.isValid()) {
      tft.print("Date: ");
      tft.setTextColor(ILI9341_WHITE);
      tft.print(gps.date.day());
      tft.print("/");
      tft.print(gps.date.month());
      tft.print("/");
      tft.println(gps.date.year());

      tft.setCursor(20, 200);
      tft.setTextColor(ILI9341_CYAN);
      tft.print("Time: ");
      tft.setTextColor(ILI9341_WHITE);
      tft.print(gps.time.hour());
      tft.print(":");
      tft.print(gps.time.minute());
      tft.print(":");
      tft.println(gps.time.second());
    } else {
      tft.setTextColor(ILI9341_RED);
      tft.println("Date/Time: INVALID");
    }
  }

  delay(1000);  // Update every second
}
