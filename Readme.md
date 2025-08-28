# ESP32 Setup Guide for Home Automation PWA

## Quick Start Checklist

### 📋 **Prerequisites**
- [ ] ESP32 development board
- [ ] Arduino IDE installed
- [ ] USB cable for ESP32
- [ ] Relay modules (3.3V or 5V compatible)
- [ ] Basic electronics knowledge

### 📚 **Required Libraries**
Install these libraries through Arduino IDE Library Manager:

1. **ESPAsyncWebServer** by me-no-dev
2. **AsyncTCP** by me-no-dev  
3. **ArduinoJson** by Benoit Blanchon

### ⚙️ **Arduino IDE Configuration**

1. **Add ESP32 Board Package:**
   - Go to File → Preferences
   - Add this URL to "Additional Board Manager URLs":
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Go to Tools → Board → Board Manager
   - Search for "ESP32" and install "esp32 by Espressif Systems"

2. **Board Settings:**
   - Board: "ESP32 Dev Module"
   - Upload Speed: 921600
   - CPU Frequency: 240MHz (WiFi/BT)
   - Flash Frequency: 80MHz
   - Flash Mode: QIO
   - Flash Size: 4MB (32Mb)
   - Partition Scheme: "Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)"

### 🔧 **Hardware Setup**

#### **Basic Relay Module Wiring:**
```
ESP32          Relay Module
-----          ------------
3.3V or 5V  →  VCC
GND         →  GND
GPIO 23     →  IN1
GPIO 25     →  IN2
GPIO 26     →  IN3
GPIO 27     →  IN4
```

#### **Safety Notes:**
⚠️ **IMPORTANT:** 
- Use optoisolated relays for AC loads
- Follow local electrical codes
- Test with low voltage loads first
- Use proper enclosures for final installation
- Consider GFCI protection for wet locations

### 💾 **Software Installation**

#### **Step 1: Upload Arduino Sketch**
1. Open `HomeAutomationESP32.ino` in Arduino IDE
2. **IMPORTANT:** Modify these settings at the top of the file:
   ```cpp
   const char* DEFAULT_SSID = "YourWiFiNetwork";
   const char* DEFAULT_PASSWORD = "YourWiFiPassword";
   const String DEFAULT_API_KEY = "your-secure-api-key-here";
   const String OTA_PASSWORD = "ota-update-password";
   ```
3. Connect ESP32 via USB
4. Select correct COM port in Tools → Port
5. Click Upload button

#### **Step 2: Upload PWA Files to SPIFFS**
1. Install "ESP32 Sketch Data Upload" tool:
   - Download from: https://github.com/me-no-dev/arduino-esp32fs-plugin
   - Extract to `[Arduino]/tools/ESP32FS/tool/esp32fs.jar`
   - Restart Arduino IDE

2. Create data folder structure:
   ```
   [Sketch Folder]/
   ├── HomeAutomationESP32.ino
   └── data/
       ├── index.html
       ├── manifest.json
       ├── service-worker.js
       ├── css/
       │   └── styles.css
       ├── js/
       │   └── app.js
       └── icons/
           ├── icon-72x72.png
           ├── icon-96x96.png
           └── ... (all icon files)
   ```

3. Upload SPIFFS data:
   - In Arduino IDE: Tools → ESP32 Sketch Data Upload
   - Wait for upload to complete

### 🌐 **First Time Setup**

#### **Method 1: Successful WiFi Connection**
If WiFi credentials are correct:
1. Open Serial Monitor (115200 baud)
2. Note the IP address displayed
3. Open `http://[ESP32_IP]` in browser
4. PWA should load successfully

#### **Method 2: Access Point Mode**
If WiFi fails to connect:
1. ESP32 creates hotspot: `HomeAutomation-Setup`
2. Password: `homeauto123`
3. Connect to this network
4. Open `http://192.168.4.1`
5. Configure WiFi settings through web interface
6. Device will restart and connect to your network

### 📱 **PWA Configuration**

1. **Open PWA in browser**
2. **Click settings gear icon (⚙️)**
3. **Configure Network Settings:**
   - Local IP: Your ESP32's local IP (e.g., `192.168.1.200`)
   - Remote IP: Leave blank unless using VPN
   - API Key: Same as set in Arduino sketch

4. **Add Rooms and Devices:**
   - Add rooms (Living Room, Bedroom, etc.)
   - Add devices with correct GPIO pins
   - Choose appropriate icons

5. **Test Connection:**
   - Click "Test Connection" button
   - Should show "Connection successful!"

6. **Install PWA:**
   - Mobile: Browser menu → "Add to Home Screen"
   - Desktop: Browser will show install prompt

### 🔍 **Troubleshooting**

#### **ESP32 Won't Connect to WiFi**
- Check SSID and password in code
- Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
- Check WiFi signal strength
- Try Access Point mode for configuration

#### **PWA Shows Connection Error**
- Verify ESP32 IP address
- Check API key matches
- Ensure ESP32 and browser device are on same network
- Check firewall settings

#### **Device Controls Don't Work**
- Verify GPIO pin numbers in both ESP32 code and PWA settings
- Check relay module connections
- Monitor Serial output for error messages
- Test individual GPIOs with simple digitalWrite commands

#### **SPIFFS Upload Fails**
- Ensure correct partition scheme selected
- Close Serial Monitor before uploading
- Try different USB cable/port
- Restart Arduino IDE

### 🛡️ **Security Configuration**

#### **Change Default Passwords:**
```cpp
// In Arduino sketch
const String DEFAULT_API_KEY = "create-a-strong-random-key-here";
const String OTA_PASSWORD = "your-ota-password";
const char* AP_PASSWORD = "your-ap-password";
```

#### **Network Security:**
- Use strong WiFi passwords
- Consider IoT device VLAN isolation
- Disable WPS on router
- Enable WPA3 if available

### 📊 **Monitoring and Maintenance**

#### **Serial Monitor Output:**
Enable Serial Monitor (115200 baud) to see:
- Device initialization status
- WiFi connection details
- API request logs
- Error messages

#### **Web-based Monitoring:**
- Access `http://[ESP32_IP]/api/info` for system information
- Check device states via `http://[ESP32_IP]/api/status`
- Monitor memory usage and uptime

#### **OTA Updates:**
- Enabled by default for remote firmware updates
- Use Arduino IDE: Tools → Port → [ESP32_IP] (network port)
- Enter OTA password when prompted

### 🔧 **Advanced Configuration**

#### **Adding More Devices:**
Modify the DEFAULT_DEVICES array in Arduino sketch:
```cpp
const struct {
  int gpio;
  const char* name;
  const char* room;
  const char* icon;
} DEFAULT_DEVICES[] = {
  {23, "Living Room Light", "living-room", "💡"},
  {25, "Bedroom Fan", "bedroom", "🪭"},
  // Add more devices here
  {32, "Garden Sprinkler", "garden", "💧"},
  {33, "Garage Door", "garage", "🚪"}
};
```

#### **WebSocket Real-time Updates:**
The ESP32 automatically sends real-time updates via WebSocket:
- Device state changes
- System heartbeat every 30 seconds
- Connection status updates

#### **Static IP Configuration:**
Uncomment and modify in Arduino sketch:
```cpp
IPAddress local_IP(192, 168, 1, 200);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

// In setup(), add before WiFi.begin():
WiFi.config(local_IP, gateway, subnet);
```

### 📋 **Final Checklist**
- [ ] ESP32 sketch uploaded successfully
- [ ] SPIFFS data uploaded
- [ ] WiFi connection established
- [ ] PWA loads in browser
- [ ] API key configured correctly
- [ ] Devices added and working
- [ ] PWA installed on mobile device
- [ ] Voice control tested (if needed)
- [ ] Offline functionality verified

### 🆘 **Support Resources**
- **ESP32 Documentation:** https://docs.espressif.com/projects/esp32/
- **Arduino ESP32 Core:** https://github.com/espressif/arduino-esp32
- **PWA Documentation:** Check README.md in project folder
- **Serial Monitor:** Essential for debugging connection issues

---

**Your home automation system is now ready! 🏠✨**
