/*
 * Triple Connection Arduino Dial for Arduino UNO R4 WiFi
 * Connection priority: Serial > Bluetooth BLE > WiFi HTTP
 * Clearly indicates which connection method is active
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

void setup() {
  Serial.begin(9600);
  delay(2000);
  
  Serial.println("=== Triple Connection Arduino Dial ===");
  Serial.println("Connection Priority: Serial > Bluetooth > WiFi");
  
  // Initialize Modulino
  Modulino.begin();
  knob.begin();
  lastPosition = knob.get();
  
  // Try to establish connection (Serial > Bluetooth > WiFi)
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
  Serial.println("Establishing connection...");
  
  // Priority 1: Try Serial connection
  if (checkSerialConnection()) {
    activeConnection = CONNECTION_SERIAL;
    serialAvailable = true;
    Serial.println(">>> CONNECTION: SERIAL ACTIVE <<<");
    Serial.println("Dial ready! Using Serial communication.");
    return;
  }
  
  // Priority 2: Try Bluetooth BLE
  Serial.println("Serial not available, trying Bluetooth BLE...");
  if (connectToBluetooth()) {
    activeConnection = CONNECTION_BLUETOOTH;
    bluetoothConnected = true;
    Serial.println(">>> CONNECTION: BLUETOOTH BLE ACTIVE <<<");
    Serial.println("Dial ready! Using Bluetooth BLE communication.");
    Serial.println("Device name: Arduino Dial");
    return;
  }
  
  // Priority 3: Try WiFi
  Serial.println("Bluetooth not available, trying WiFi...");
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
  delay(5000);
}

bool checkSerialConnection() {
  // Simple check for Serial availability
  // In production, you might implement a more sophisticated handshake
  return Serial && Serial.availableForWrite() > 0;
}

bool connectToBluetooth() {
  Serial.println("Initializing Bluetooth BLE...");
  
  // Initialize BLE
  if (!BLE.begin()) {
    Serial.println("Starting Bluetooth BLE failed!");
    return false;
  }
  
  // Set BLE device name and local name
  BLE.setLocalName("Arduino Dial");
  BLE.setDeviceName("Arduino Dial");
  
  // Add characteristics to service
  dialService.addCharacteristic(dialPositionChar);
  dialService.addCharacteristic(dialClickChar);
  
  // Add service
  BLE.addService(dialService);
  
  // Set initial values
  dialPositionChar.writeValue(lastPosition);
  dialClickChar.writeValue(false);
  
  // Start advertising
  BLE.advertise();
  
  Serial.println("Bluetooth BLE peripheral started");
  Serial.println("Device name: Arduino Dial");
  Serial.print("Service UUID: ");
  Serial.println(DIAL_SERVICE_UUID);
  
  return true;
}

bool connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
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
    Serial.println("WiFi connected successfully!");
    
    // Wait for valid IP address
    Serial.print("Waiting for IP address");
    while (WiFi.localIP().toString() == "0.0.0.0") {
      delay(500);
      Serial.print(".");
    }
    Serial.println();
    
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println();
    Serial.println("WiFi connection failed!");
    return false;
  }
}

void maintainConnection() {
  if (activeConnection == CONNECTION_SERIAL) {
    // Check if Serial is still available
    if (!Serial || !Serial.availableForWrite()) {
      Serial.println("Serial connection lost, switching to Bluetooth...");
      serialAvailable = false;
      activeConnection = CONNECTION_NONE;
      establishConnection();
    }
  } else if (activeConnection == CONNECTION_BLUETOOTH) {
    // Check if a central device is connected
    BLEDevice central = BLE.central();
    if (!central || !central.connected()) {
      // Still keep BLE active even if no central connected
      static unsigned long lastBLECheck = 0;
      if (millis() - lastBLECheck > 30000) { // Check every 30 seconds
        Serial.println("No BLE central connected, but still advertising...");
        lastBLECheck = millis();
      }
    }
  } else if (activeConnection == CONNECTION_WIFI) {
    // Check WiFi connection
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected, attempting to reconnect...");
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
  Serial.print("DIAL:");
  Serial.println(position);
}

void sendClickDataSerial() {
  Serial.println("CLICK:1");
}

void sendDialDataBluetooth(int position, int delta) {
  // Update BLE characteristic with new position
  dialPositionChar.writeValue(position);
  
  // Only print every 10th message to reduce serial spam
  static int msgCount = 0;
  if (++msgCount % 10 == 0) {
    Serial.print("Sent dial position: ");
    Serial.print(position);
    Serial.print(", delta: ");
    Serial.println(delta);
  }
}

void sendClickDataBluetooth() {
  // Send click event via BLE
  dialClickChar.writeValue(true);
  delay(50); // Brief pulse
  dialClickChar.writeValue(false);
  
  Serial.println("Sent click event");
}

void sendDialDataWiFi(int position, int delta) {
  // Check WiFi connection first
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skipping send");
    return;
  }
  
  if (WiFi.localIP().toString() == "0.0.0.0") {
    Serial.println("No valid IP address, skipping send");
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
      Serial.print("Sent dial batch, latest: pos=");
      Serial.print(position);
      Serial.print(", delta=");
      Serial.println(delta);
    }
    
  } else {
    Serial.print("Connection to server failed - Host: ");
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
    
    Serial.println("Sent click");
    
  } else {
    Serial.println("Connection to server failed");
  }
}