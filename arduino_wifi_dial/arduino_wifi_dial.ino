/*
 * WiFi Dial for Arduino UNO R4 WiFi
 * Sends dial data to webpage via HTTP POST requests
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

int lastPosition = 0;
unsigned long lastSendTime = 0;
const int SEND_INTERVAL = 30; // Send every 30ms for smoother response

void setup() {
  Serial.begin(9600);
  delay(2000);
  
  Serial.println("=== WiFi Dial Setup ===");
  
  // Initialize Modulino
  Modulino.begin();
  knob.begin();
  lastPosition = knob.get();
  
  // Connect to WiFi
  Serial.print("Connecting to ");
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
    Serial.print("Sending dial data to: http://");
    Serial.print(serverHost);
    Serial.print(":");
    Serial.println(serverPort);
    Serial.println("Dial ready!");
  } else {
    Serial.println();
    Serial.println("WiFi connection failed!");
    Serial.println("Check your credentials and try again.");
    return;
  }
}

void loop() {
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Attempting to reconnect...");
    WiFi.begin(ssid, password);
    delay(5000);
    return;
  }
  
  unsigned long currentTime = millis();
  
  // Read current knob state
  int currentPosition = knob.get();
  bool clicked = knob.isPressed();
  
  // Send dial data when position changes or at regular intervals
  if (currentPosition != lastPosition) {
    // Calculate delta for smooth scrolling
    int delta = currentPosition - lastPosition;
    sendDialData(currentPosition, delta);
    lastPosition = currentPosition;
    lastSendTime = currentTime;
  } else if ((currentTime - lastSendTime) >= SEND_INTERVAL) {
    // Send periodic updates to keep connection alive
    lastSendTime = currentTime;
  }
  
  // Handle click events (for step navigation)
  if (clicked) {
    sendClickData();
    delay(200); // Debounce click
  }
  
  delay(5); // Minimal delay for faster response
}

void sendDialData(int position, int delta) {
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

void sendClickData() {
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