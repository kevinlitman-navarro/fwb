#include <Modulino.h>

ModulinoKnob knob;

int lastPosition = 0;
unsigned long lastSendTime = 0;
const int SEND_INTERVAL = 50; // Send every 50ms for smooth scrolling

void setup() {
   Serial.begin(9600);
   Modulino.begin();
   knob.begin();
   
   // Get initial position
   lastPosition = knob.get();
   
   Serial.println("Knob Modulino Ready for Web Integration");
}

void loop(){
   unsigned long currentTime = millis();
   
   // Read current knob state
   int currentPosition = knob.get();
   bool clicked = knob.isPressed();
   
   // Send data on any position change for maximum responsiveness
   if (currentPosition != lastPosition || (currentTime - lastSendTime) >= SEND_INTERVAL) {
      
      // Send in format expected by JavaScript: "DIAL:value"
      Serial.print("DIAL:");
      Serial.println(currentPosition);
      
      lastPosition = currentPosition;
      lastSendTime = currentTime;
   }
   
   // Handle click events (could be used for step navigation)
   if (clicked) {
      Serial.println("CLICK:1");
      delay(200); // Debounce click
   }
   
   delay(10); // Small delay for stability
}