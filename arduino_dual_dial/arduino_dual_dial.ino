/*
 * Dual Connection Arduino Dial for Arduino UNO R4 WiFi
 * Automatically uses Serial connection by default, falls back to WiFi if Serial unavailable
 * Clearly indicates which connection method is active
 */

#include <WiFiS3.h>
#include <Modulino.h>

// WiFi credentials - UPDATE THESE TO MATCH YOUR NETWORK
const char* ssid = "Happy Birthday Charlie";
const char* password = "msgilpin";

// Web server details - UPDATE THIS TO YOUR LOCAL IP
const char* serverHost = "192.168.1.169"; // Change to your computer's local IP
const int serverPort = 5173; // SvelteKit dev server default port

ModulinoKnob knob;
WiFiClient client;

// Connection state
enum ConnectionType {
  CONNECTION_NONE,
  CONNECTION_SERIAL,
  CONNECTION_WIFI
};

ConnectionType activeConnection = CONNECTION_NONE;
bool serialAvailable = false;
bool wifiConnected = false;

int lastPosition = 0;
unsigned long lastSendTime = 0;
unsigned long lastConnectionCheck = 0;
const int SEND_INTERVAL = 50; // Send every 50ms
const int CONNECTION_CHECK_INTERVAL = 5000; // Check connection every 5 seconds

void setup() {
  Serial.begin(9600);
  delay(2000);
  
  Serial.println("=== Dual Connection Arduino Dial ===");
  
  // Initialize Modulino
  Modulino.begin();
  knob.begin();
  lastPosition = knob.get();
  
  // Try to establish connection (Serial first, then WiFi)
  establishConnection();
}

void loop() {
  unsigned long currentTime = millis();
  
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
  
  // First, try Serial connection
  if (checkSerialConnection()) {
    activeConnection = CONNECTION_SERIAL;
    serialAvailable = true;
    Serial.println(">>> CONNECTION: SERIAL ACTIVE <<<");
    Serial.println("Dial ready! Using Serial communication.");
    return;
  }
  
  // If Serial fails, try WiFi
  Serial.println("Serial connection not available, trying WiFi...");
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
  
  // If both fail
  activeConnection = CONNECTION_NONE;
  Serial.println(">>> CONNECTION: NONE - RETRYING IN 5 SECONDS <<<");
  delay(5000);
}

bool checkSerialConnection() {
  // Send a test message and wait for any response or activity
  // This is a simple check - in a real scenario, you might implement
  // a handshake protocol with the host computer
  
  Serial.println("SERIAL_TEST");
  delay(100);
  
  // For now, we'll assume Serial is available if the Serial object is ready
  // In practice, you might want to implement a more sophisticated check
  return Serial;
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
    if (!Serial) {
      Serial.println("Serial connection lost, switching to WiFi...");
      serialAvailable = false;
      activeConnection = CONNECTION_NONE;
      establishConnection();
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
  if (activeConnection == CONNECTION_SERIAL) {
    sendDialDataSerial(position, delta);
  } else if (activeConnection == CONNECTION_WIFI) {
    sendDialDataWiFi(position, delta);
  }
}

void sendClickData() {
  if (activeConnection == CONNECTION_SERIAL) {
    sendClickDataSerial();
  } else if (activeConnection == CONNECTION_WIFI) {
    sendClickDataWiFi();
  }
}

void sendDialDataSerial(int position, int delta) {
  // Send in format expected by JavaScript: "DIAL:value"
  Serial.print("DIAL:");
  Serial.println(position);
}

void sendClickDataSerial() {
  Serial.println("CLICK:1");
}

void sendDialDataWiFi(int position, int delta) {
  // Check WiFi connection first
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skipping send");
    return;
  }
  
  // Check if we have a valid IP
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
    
    // Skip reading response for speed
    client.stop();
    
    // Only print every 10th message to reduce serial spam
    static int msgCount = 0;
    if (++msgCount % 10 == 0) {
      Serial.print("[WiFi] Sent dial batch, latest: pos=");
      Serial.print(position);
      Serial.print(", delta=");
      Serial.println(delta);
    }
    
  } else {
    Serial.print("[WiFi] Connection to server failed - Host: ");
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
    
    Serial.println("[WiFi] Sent click");
    
  } else {
    Serial.println("[WiFi] Connection to server failed");
  }
}