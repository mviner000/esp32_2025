// ESP32 code to control a solenoid lock using a button and LED indicator
// No WiFi implementation as requested
// Using non-blocking timing (no delays)

// Pin definitions
const int RELAY_PIN = 13;     // Relay control pin
const int LED_PIN = 5;        // Status LED pin
const int BUTTON_PIN = 14;    // Button input pin

// Variables
bool lockState = true;        // true = locked, false = unlocked
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
int lastButtonState = HIGH;
int buttonState = HIGH;

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  Serial.println("Solenoid Lock Control System");
  
  // Configure pins
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Initialize the lock in locked state (relay OFF)
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(LED_PIN, LOW);  // LED off when locked
  
  Serial.println("System initialized - Lock is secured");
}

void loop() {
  // Handle button input with debouncing
  handleButton();
}

void handleButton() {
  // Read the button state
  int reading = digitalRead(BUTTON_PIN);
  
  // Check if button state changed
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  // Debounce the button
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      
      // Toggle lock on button press (when button is LOW)
      if (buttonState == LOW) {
        toggleLock();
      }
    }
  }
  
  lastButtonState = reading;
}

void toggleLock() {
  lockState = !lockState;
  
  if (lockState) {
    // Lock the door
    digitalWrite(RELAY_PIN, LOW);  // Relay OFF
    digitalWrite(LED_PIN, LOW);    // LED OFF
    Serial.println("Door locked");
  } else {
    // Unlock the door
    digitalWrite(RELAY_PIN, HIGH); // Relay ON
    digitalWrite(LED_PIN, HIGH);   // LED ON
    Serial.println("Door unlocked");
  }
}