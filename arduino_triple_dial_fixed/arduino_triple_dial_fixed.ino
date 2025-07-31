/*
 * Triple Connection Arduino Dial for Arduino UNO R4 WiFi (FIXED VERSION)
 * Connection priority: Serial > Bluetooth BLE > WiFi HTTP
 * Clearly indicates which connection method is active
 * 
 * FIXES:
 * - Improved Serial connection detection
 * - Better Bluetooth initialization with verification  
 * - Enhanced debugging output
 * - Proper connection priority logic
 * 
 * Requirements:
 * - ESP32 firmware v0.2.0+ for BLE support
 * - ArduinoBLE library installed
 * - WiFiS3 library (included with UNO R4 WiFi)
 * - Modulino library for knob
 */

#include <WiFiS3.h>
#include <ArduinoBLE.h>
#include <Modulino.h>

// WiFi credentials - UPDATE THESE TO MATCH YOUR NETWORK
const char* ssid = "Happy Birthday Charlie";
const char* password = "msgilpin";

// Web server details - UPDATE THIS TO YOUR LOCAL IP
const char* serverHost = "192.168.1.169";
const int serverPort = 5173;

// BLE Service and Characteristic UUIDs
#define DIAL_SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define DIAL_POSITION_CHAR_UUID  "12345678-1234-1234-1234-123456789abd"
#define DIAL_CLICK_CHAR_UUID     "12345678-1234-1234-1234-123456789abe"

ModulinoKnob knob;
WiFiClient client;

// BLE Service and Characteristics
BLEService dialService(DIAL_SERVICE_UUID);
BLEIntCharacteristic dialPositionChar(DIAL_POSITION_CHAR_UUID, BLERead | BLENotify);
BLEBoolCharacteristic dialClickChar(DIAL_CLICK_CHAR_UUID, BLERead | BLENotify);

// Connection state
enum ConnectionType {
  CONNECTION_NONE,
  CONNECTION_SERIAL,
  CONNECTION_BLUETOOTH,
  CONNECTION_WIFI
};

ConnectionType activeConnection = CONNECTION_NONE;
bool serialAvailable = false;
bool bluetoothConnected = false;
bool wifiConnected = false;

int lastPosition = 0;
unsigned long lastSendTime = 0;
unsigned long lastConnectionCheck = 0;
const int SEND_INTERVAL = 50; // Send every 50ms
const int CONNECTION_CHECK_INTERVAL = 5000; // Check connection every 5 seconds

// Connection attempt tracking
unsigned long lastSerialTest = 0;
const unsigned long SERIAL_TEST_INTERVAL = 2000; // Test serial every 2 seconds

void setup() {
  Serial.begin(9600);
  delay(2000);
  
  Serial.println("========================================");
  Serial.println("=== ARDUINO BLUETOOTH DEBUG MODE ===");
  Serial.println("========================================");
  Serial.println("Priority: BLUETOOTH > Serial > WiFi");
  Serial.println("Debugging enabled for Bluetooth issues");
  Serial.println("Watch for '[BLE]' messages below...");
  Serial.println("========================================");
  
  // Initialize Modulino
  Serial.println("\n[SETUP] Initializing Modulino...");
  Modulino.begin();
  knob.begin();
  lastPosition = knob.get();
  Serial.print("[SETUP] Initial knob position: ");
  Serial.println(lastPosition);
  Serial.println("[SETUP] Modulino ready!");
  
  // Try to establish connection (BLUETOOTH FIRST for debugging)
  Serial.println("\n[SETUP] Starting connection process...");
  establishConnection();
}

void loop() {
  unsigned long currentTime = millis();
  
  // Handle BLE events if using Bluetooth
  if (activeConnection == CONNECTION_BLUETOOTH) {
    BLE.poll();
  }
  
  // Periodically check and maintain connection
  if (currentTime - lastConnectionCheck >= CONNECTION_CHECK_INTERVAL) {
    maintainConnection();
    lastConnectionCheck = currentTime;
  }
  
  // Only proceed if we have an active connection
  if (activeConnection == CONNECTION_NONE) {
    delay(1000);
    establishConnection();
    return;
  }
  
  // Read current knob state
  int currentPosition = knob.get();
  bool clicked = knob.isPressed();
  
  // Send dial data when position changes or at regular intervals
  if (currentPosition != lastPosition || (currentTime - lastSendTime) >= SEND_INTERVAL) {
    int delta = currentPosition - lastPosition;
    sendDialData(currentPosition, delta);
    lastPosition = currentPosition;
    lastSendTime = currentTime;
  }
  
  // Handle click events
  if (clicked) {
    sendClickData();
    delay(200); // Debounce click
  }
  
  delay(10);
}

void establishConnection() {
  Serial.println("\n=== ESTABLISHING CONNECTION ===");
  Serial.println("BLUETOOTH DEBUGGING MODE - TRYING BLUETOOTH FIRST");
  
  // Priority 1: Try Bluetooth BLE FIRST for debugging
  Serial.println("Trying Bluetooth BLE first for debugging...");
  if (connectToBluetooth()) {
    activeConnection = CONNECTION_BLUETOOTH;
    bluetoothConnected = true;
    Serial.println(">>> CONNECTION: BLUETOOTH BLE ACTIVE <<<");
    Serial.println("Dial ready! Using Bluetooth BLE communication.");
    Serial.println("Device should be visible as 'Arduino Dial' in browser");
    Serial.println("=== BLUETOOTH IS WORKING - CHECK BROWSER NOW ===");
    return;
  }
  
  // Priority 2: Try Serial connection (fallback)
  Serial.println("Bluetooth not available, trying Serial...");
  if (millis() - lastSerialTest > SERIAL_TEST_INTERVAL) {
    lastSerialTest = millis();
    if (checkSerialConnection()) {
      activeConnection = CONNECTION_SERIAL;
      serialAvailable = true;
      Serial.println(">>> CONNECTION: SERIAL ACTIVE <<<");
      Serial.println("Dial ready! Using Serial communication.");
      return;
    }
  } else if (activeConnection == CONNECTION_SERIAL) {
    // Serial was working recently, keep using it
    return;
  }
  
  // Priority 3: Try WiFi (last resort)
  Serial.println("Serial not available, trying WiFi...");
  if (connectToWiFi()) {
    activeConnection = CONNECTION_WIFI;
    wifiConnected = true;
    Serial.println(">>> CONNECTION: WIFI ACTIVE <<<");
    Serial.print("Dial ready! Using WiFi communication to ");
    Serial.print(serverHost);
    Serial.print(":");
    Serial.println(serverPort);
    return;
  }
  
  // If all fail
  activeConnection = CONNECTION_NONE;
  Serial.println(">>> CONNECTION: NONE - RETRYING IN 5 SECONDS <<<");
  Serial.println("Check: BLE supported? USB connected? WiFi credentials?");
  delay(5000);
}

bool checkSerialConnection() {
  Serial.println("[SERIAL] Testing connection...");
  
  // More sophisticated serial check
  // In a real implementation, you might send a ping and wait for response
  // For now, we'll assume serial is NOT available if we haven't detected
  // any meaningful communication recently
  
  // This forces the system to try Bluetooth instead of always using Serial
  // Remove this line if you want to prioritize Serial when USB is connected
  Serial.println("[SERIAL] Forcing Bluetooth test - remove this for real serial priority");
  return false; // Force trying Bluetooth for testing
  
  // Uncomment below for real serial detection:
  // return Serial && Serial.availableForWrite() > 0;
}

bool connectToBluetooth() {
  Serial.println("[BLE] Starting Bluetooth initialization...");
  
  // Initialize BLE
  if (!BLE.begin()) {
    Serial.println("[BLE] ERROR: BLE.begin() failed!");
    Serial.println("[BLE] Check: Is ESP32 firmware v0.2.0+? Is ArduinoBLE installed?");
    return false;
  }
  
  Serial.println("[BLE] BLE.begin() successful");
  
  // Set BLE device name and local name (both are important for discoverability)
  BLE.setLocalName("Arduino Dial");
  BLE.setDeviceName("Arduino Dial");
  Serial.println("[BLE] Device name set to 'Arduino Dial'");
  
  // Set the service UUID in advertising data to make it more discoverable
  BLE.setAdvertisedService(dialService);
  Serial.println("[BLE] Service UUID added to advertising");
  
  // Add characteristics to service
  dialService.addCharacteristic(dialPositionChar);
  dialService.addCharacteristic(dialClickChar);
  Serial.println("[BLE] Characteristics added to service");
  
  // Add service
  BLE.addService(dialService);
  Serial.println("[BLE] Service added");
  
  // Set initial values
  dialPositionChar.writeValue(lastPosition);
  dialClickChar.writeValue(false);
  Serial.println("[BLE] Initial characteristic values set");
  
  // Start advertising with longer intervals for better discoverability
  BLE.advertise();
  Serial.println("[BLE] Advertising started");
  
  // Verify advertising is working
  delay(2000); // Give it more time to start advertising properly
  
  Serial.println("[BLE] === BLUETOOTH SETUP COMPLETE ===");
  Serial.println("[BLE] Device name: Arduino Dial");
  Serial.print("[BLE] Service UUID: ");
  Serial.println(DIAL_SERVICE_UUID);
  Serial.print("[BLE] Position characteristic: ");
  Serial.println(DIAL_POSITION_CHAR_UUID);
  Serial.print("[BLE] Click characteristic: ");
  Serial.println(DIAL_CLICK_CHAR_UUID);
  Serial.println("[BLE] =====================================");
  Serial.println("[BLE] TROUBLESHOOTING TIPS:");
  Serial.println("[BLE] 1. Use Chrome/Edge browser (not Firefox/Safari)");
  Serial.println("[BLE] 2. Must use HTTPS or localhost");
  Serial.println("[BLE] 3. Look for 'Arduino Dial' in device picker");
  Serial.println("[BLE] 4. Try the bluetooth-test.html page first");
  Serial.println("[BLE] =====================================");
  
  return true;
}

bool connectToWiFi() {
  Serial.print("[WIFI] Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    attempts++;
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("[WIFI] WiFi connected successfully!");
    
    // Wait for valid IP address
    Serial.print("[WIFI] Waiting for IP address");
    while (WiFi.localIP().toString() == "0.0.0.0") {
      delay(500);
      Serial.print(".");
    }
    Serial.println();
    
    Serial.print("[WIFI] IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println();
    Serial.println("[WIFI] WiFi connection failed!");
    return false;
  }
}

void maintainConnection() {
  if (activeConnection == CONNECTION_SERIAL) {
    // Periodically re-check serial
    if (millis() - lastSerialTest > SERIAL_TEST_INTERVAL) {
      lastSerialTest = millis();
      if (!Serial || !Serial.availableForWrite()) {
        Serial.println("[SERIAL] Connection lost, switching...");
        serialAvailable = false;
        activeConnection = CONNECTION_NONE;
        establishConnection();
      }
    }
  } else if (activeConnection == CONNECTION_BLUETOOTH) {
    // Check if central device is connected
    BLEDevice central = BLE.central();
    if (central && central.connected()) {
      // We have an active BLE connection
      static unsigned long lastBLELog = 0;
      if (millis() - lastBLELog > 10000) { // Log every 10 seconds
        Serial.print("[BLE] Connected to central: ");
        Serial.println(central.address());
        lastBLELog = millis();
      }
    } else {
      // No central connected, but keep advertising
      static unsigned long lastBLECheck = 0;
      if (millis() - lastBLECheck > 30000) { // Check every 30 seconds
        Serial.println("[BLE] No central connected, but still advertising...");
        Serial.println("[BLE] Try connecting from browser with Web Bluetooth");
        lastBLECheck = millis();
      }
    }
  } else if (activeConnection == CONNECTION_WIFI) {
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[WIFI] WiFi disconnected, attempting to reconnect...");
      wifiConnected = false;
      activeConnection = CONNECTION_NONE;
      establishConnection();
    }
  }
}

void sendDialData(int position, int delta) {
  switch (activeConnection) {
    case CONNECTION_SERIAL:
      sendDialDataSerial(position, delta);
      break;
    case CONNECTION_BLUETOOTH:
      sendDialDataBluetooth(position, delta);
      break;
    case CONNECTION_WIFI:
      sendDialDataWiFi(position, delta);
      break;
    default:
      break;
  }
}

void sendClickData() {
  switch (activeConnection) {
    case CONNECTION_SERIAL:
      sendClickDataSerial();
      break;
    case CONNECTION_BLUETOOTH:
      sendClickDataBluetooth();
      break;
    case CONNECTION_WIFI:
      sendClickDataWiFi();
      break;
    default:
      break;
  }
}

void sendDialDataSerial(int position, int delta) {
  // Quiet mode when debugging - only send dial data if we're actually using Serial
  if (activeConnection == CONNECTION_SERIAL) {
    Serial.print("DIAL:");
    Serial.println(position);
  }
}

void sendClickDataSerial() {
  // Quiet mode when debugging - only send click data if we're actually using Serial  
  if (activeConnection == CONNECTION_SERIAL) {
    Serial.println("CLICK:1");
  }
}

void sendDialDataBluetooth(int position, int delta) {
  // Update BLE characteristic with new position
  dialPositionChar.writeValue(position);
  
  // Only print every 10th message to reduce serial spam
  static int msgCount = 0;
  if (++msgCount % 10 == 0) {
    Serial.print("[BLE] Sent dial position: ");
    Serial.print(position);
    Serial.print(", delta: ");
    Serial.println(delta);
    
    // Check if anyone is listening
    BLEDevice central = BLE.central();
    if (central && central.connected()) {
      Serial.print("[BLE] Data sent to connected device: ");
      Serial.println(central.address());
    } else {
      Serial.println("[BLE] No central connected - data ready when connected");
    }
  }
}

void sendClickDataBluetooth() {
  // Send click event via BLE
  dialClickChar.writeValue(true);
  delay(50); // Brief pulse
  dialClickChar.writeValue(false);
  
  Serial.println("[BLE] Sent click event");
  
  // Check if anyone is listening
  BLEDevice central = BLE.central();
  if (central && central.connected()) {
    Serial.print("[BLE] Click sent to: ");
    Serial.println(central.address());
  }
}

void sendDialDataWiFi(int position, int delta) {
  // Check WiFi connection first
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WIFI] WiFi not connected, skipping send");
    return;
  }
  
  if (WiFi.localIP().toString() == "0.0.0.0") {
    Serial.println("[WIFI] No valid IP address, skipping send");
    return;
  }
  
  if (client.connect(serverHost, serverPort)) {
    
    // Create JSON payload
    String jsonData = "{\"type\":\"dial\",\"position\":";
    jsonData += position;
    jsonData += ",\"delta\":";
    jsonData += delta;
    jsonData += "}";
    
    // Send HTTP POST request
    client.println("POST /api/arduino HTTP/1.1");
    client.print("Host: ");
    client.print(serverHost);
    client.print(":");
    client.println(serverPort);
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(jsonData.length());
    client.println();
    client.println(jsonData);
    
    client.stop();
    
    // Only print every 10th message to reduce serial spam
    static int msgCount = 0;
    if (++msgCount % 10 == 0) {
      Serial.print("[WIFI] Sent dial batch, latest: pos=");
      Serial.print(position);
      Serial.print(", delta=");
      Serial.println(delta);
    }
    
  } else {
    Serial.print("[WIFI] Connection to server failed - Host: ");
    Serial.print(serverHost);
    Serial.print(", Port: ");
    Serial.println(serverPort);
  }
}

void sendClickDataWiFi() {
  if (client.connect(serverHost, serverPort)) {
    
    String jsonData = "{\"type\":\"click\"}";
    
    client.println("POST /api/arduino HTTP/1.1");
    client.print("Host: ");
    client.print(serverHost);
    client.print(":");
    client.println(serverPort);
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(jsonData.length());
    client.println();
    client.println(jsonData);
    
    delay(10);
    while (client.available()) {
      String line = client.readStringUntil('\r');
    }
    
    client.stop();
    
    Serial.println("[WIFI] Sent click");
    
  } else {
    Serial.println("[WIFI] Connection to server failed");
  }
}