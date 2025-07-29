/*
 * WiFi Connection Debugger for Arduino UNO R4 WiFi
 * Helps diagnose WiFi connection issues
 */

#include <WiFiS3.h>

// WiFi credentials - UPDATE THESE
const char* ssid = "Happy Birthday Charlie";
const char* password = "msgilpin";

void setup() {
  Serial.begin(9600);
  delay(2000);
  
  Serial.println("=== WiFi Connection Debugger ===");
  Serial.print("Attempting to connect to: ");
  Serial.println(ssid);
  Serial.print("Password length: ");
  Serial.println(strlen(password));
  Serial.println();
  
  // Scan for available networks first
  Serial.println("Scanning for available WiFi networks...");
  int numNetworks = WiFi.scanNetworks();
  
  if (numNetworks == 0) {
    Serial.println("No networks found!");
  } else {
    Serial.print("Found ");
    Serial.print(numNetworks);
    Serial.println(" networks:");
    
    for (int i = 0; i < numNetworks; i++) {
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (Signal: ");
      Serial.print(WiFi.RSSI(i));
      Serial.print(" dBm, Security: ");
      Serial.print(WiFi.encryptionType(i));
      Serial.println(")");
      
      // Check if our target network is found
      if (WiFi.SSID(i) == ssid) {
        Serial.println("   *** TARGET NETWORK FOUND! ***");
      }
    }
  }
  
  Serial.println();
  Serial.println("Attempting connection...");
  
  // Try to connect
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    attempts++;
    Serial.print("Attempt ");
    Serial.print(attempts);
    Serial.print(": Status = ");
    printWiFiStatus(WiFi.status());
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("SUCCESS! WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Subnet mask: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("DNS: ");
    Serial.println(WiFi.dnsIP());
  } else {
    Serial.println();
    Serial.println("FAILED to connect to WiFi!");
    Serial.print("Final status: ");
    printWiFiStatus(WiFi.status());
    Serial.println();
    Serial.println("Troubleshooting tips:");
    Serial.println("1. Check SSID spelling (case sensitive)");
    Serial.println("2. Check password (case sensitive)");
    Serial.println("3. Make sure network is 2.4GHz (not 5GHz only)");
    Serial.println("4. Try moving closer to router");
    Serial.println("5. Check if network has MAC address filtering");
  }
}

void loop() {
  // Show connection status every 5 seconds
  delay(5000);
  Serial.print("Current WiFi status: ");
  printWiFiStatus(WiFi.status());
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(" - IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(" - Not connected");
  }
}

void printWiFiStatus(int status) {
  switch (status) {
    case WL_IDLE_STATUS:
      Serial.print("IDLE");
      break;
    case WL_NO_SSID_AVAIL:
      Serial.print("NO_SSID_AVAILABLE");
      break;
    case WL_SCAN_COMPLETED:
      Serial.print("SCAN_COMPLETED");
      break;
    case WL_CONNECTED:
      Serial.print("CONNECTED");
      break;
    case WL_CONNECT_FAILED:
      Serial.print("CONNECT_FAILED");
      break;
    case WL_CONNECTION_LOST:
      Serial.print("CONNECTION_LOST");
      break;
    case WL_DISCONNECTED:
      Serial.print("DISCONNECTED");
      break;
    default:
      Serial.print("UNKNOWN(");
      Serial.print(status);
      Serial.print(")");
      break;
  }
}