/*
 * Arduino UNO R4 WiFi + Knob Modulino - Simple HTTP Version
 * Creates a simple HTTP server that sends dial data
 * No extra libraries needed - uses built-in WiFiS3
 */

#include <Modulino.h>
#include <WiFiS3.h>

// WiFi credentials - UPDATE THESE WITH YOUR NETWORK
const char* ssid = "Happy Birthday Charlie";
const char* password = "msgilpin";

// Create WiFi server on port 80
WiFiServer server(80);

// Knob setup
ModulinoKnob knob;

// Dial tracking
int lastPosition = 0;
int currentPosition = 0;

void setup() {
  Serial.begin(9600);
  
  // Initialize Modulino
  Modulino.begin();
  knob.begin();
  
  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // Start HTTP server
  server.begin();
  Serial.println("HTTP server started");
  Serial.println("Access dial data at: http://[IP_ADDRESS]/dial");
  
  // Get initial position
  lastPosition = knob.get();
  currentPosition = lastPosition;
}

void loop() {
  // Update knob position
  currentPosition = knob.get();
  bool clicked = knob.isPressed();
  
  // Print to serial for debugging
  if (currentPosition != lastPosition) {
    Serial.print("DIAL:");
    Serial.println(currentPosition);
    lastPosition = currentPosition;
  }
  
  if (clicked) {
    Serial.println("CLICK:1");
    delay(200); // Debounce
  }
  
  // Handle HTTP requests
  WiFiClient client = server.available();
  if (client) {
    handleHttpRequest(client);
  }
  
  delay(10);
}

void handleHttpRequest(WiFiClient client) {
  String request = "";
  
  // Read the request
  while (client.connected() && client.available()) {
    char c = client.read();
    request += c;
    if (request.endsWith("\r\n\r\n")) {
      break;
    }
  }
  
  Serial.println("HTTP Request: " + request.substring(0, request.indexOf('\n')));
  
  // Send response based on request
  if (request.indexOf("GET /dial") >= 0) {
    // Send dial data as JSON
    sendDialData(client);
  } else if (request.indexOf("GET /") >= 0) {
    // Send simple webpage for testing
    sendWebPage(client);
  } else {
    // Send 404
    send404(client);
  }
  
  client.stop();
}

void sendDialData(WiFiClient client) {
  // Send CORS headers and JSON response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Connection: close");
  client.println();
  
  // Send dial data as JSON
  client.print("{\"dial\":");
  client.print(currentPosition);
  client.print(",\"clicked\":");
  client.print(knob.isPressed() ? "true" : "false");
  client.println("}");
}

void sendWebPage(WiFiClient client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  
  client.println("<!DOCTYPE html>");
  client.println("<html><head><title>Arduino Dial</title></head>");
  client.println("<body>");
  client.println("<h1>Arduino Knob Modulino</h1>");
  client.print("<p>Current Position: <span id='position'>");
  client.print(currentPosition);
  client.println("</span></p>");
  client.println("<p><a href='/dial'>Get Dial Data (JSON)</a></p>");
  
  client.println("<script>");
  client.println("setInterval(() => {");
  client.println("  fetch('/dial')");
  client.println("    .then(r => r.json())");
  client.println("    .then(data => {");
  client.println("      document.getElementById('position').textContent = data.dial;");
  client.println("    });");
  client.println("}, 100);");
  client.println("</script>");
  
  client.println("</body></html>");
}

void send404(WiFiClient client) {
  client.println("HTTP/1.1 404 Not Found");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.println("404 Not Found");
}

/*
 * Setup Instructions:
 * 
 * 1. Update WiFi credentials above (ssid and password)
 * 2. Upload this sketch to your Arduino UNO R4 WiFi
 * 3. Open Serial Monitor to see the IP address
 * 4. Test by visiting http://[IP_ADDRESS] in your browser
 * 5. Get dial data from http://[IP_ADDRESS]/dial
 * 
 * This creates a simple HTTP server that your web app can poll
 * for dial data instead of using WebSockets.
 * 
 * No extra libraries needed - uses built-in WiFiS3 only.
 */