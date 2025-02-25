#include <TM1637Display.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>

// GPIO Definitions for ESP32
const int CLK_PIN = 5;      // GPIO5 for TM1637 CLK
const int DIO_PIN = 4;      // GPIO4 for TM1637 DIO
const int BUZZER_PIN = 13;  // GPIO13 for buzzer
const int LED_PIN = 2;      // GPIO2 for the new LED
const int BRIGHTNESS_LEVEL = 2; // Value between 0-7

// RFID Module Pins - Changed SS_PIN to avoid conflict
#define SS_PIN  17  // Changed to GPIO17 to avoid conflict with TM1637 CLK
#define RST_PIN 27  // ESP32 GPIO27

// WiFi Settings
const char* ssid = "server_1st_esp32";  // Name of the WiFi hotspot
const char* password = "12345678";      // Password for the WiFi hotspot
IPAddress local_ip(192, 168, 4, 1);     // IP address for the access point
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// Web server on port 80
WebServer server(80);

// Create instances
TM1637Display display(CLK_PIN, DIO_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2); // Set the LCD address to 0x27 for a 16 chars and 2 line display
MFRC522 rfid(SS_PIN, RST_PIN); // Create MFRC522 instance

unsigned long totalSeconds = 60; // Start countdown from 1 minute
String lastRfidTag = ""; // Store last read RFID tag
unsigned long lastTagTime = 0; // Store time of last tag read
const unsigned long TAG_COOLDOWN = 3000; // 3 second cooldown between same tag reads

// Variables for LED control
unsigned long ledStartTime = 0;
bool ledActive = false;

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  
  // Configure pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);   // Initialize the LED pin
  digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer is off at the start
  digitalWrite(LED_PIN, LOW);    // Ensure LED is off at the start
  
  // Initialize 7-segment display
  display.setBrightness(BRIGHTNESS_LEVEL);
  displayTime(totalSeconds);
  
  // Initialize LCD
  lcd.init();                      
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Timer System");
  lcd.setCursor(0, 1);
  lcd.print("Scan RFID tag");
  
  // Initialize RFID
  SPI.begin();
  rfid.PCD_Init();
  
  // Setup WiFi Access Point
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/reset", handleReset);
  server.begin();
  
  // Show WiFi info on LCD briefly
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi: " + String(ssid));
  lcd.setCursor(0, 1);
  lcd.print("IP: 192.168.4.1");
  delay(3000);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Timer System");
  lcd.setCursor(0, 1);
  lcd.print("Scan RFID tag");
  
  Serial.println("System ready!");
  Serial.println("WiFi hotspot started: " + String(ssid));
  Serial.println("IP address: 192.168.4.1");
}

void loop() {
  // Handle web server clients
  server.handleClient();
  
  // Check for RFID card
  checkRfid();
  
  // Check if LED should be turned off (if 2 seconds have passed)
  if (ledActive && (millis() - ledStartTime >= 2000)) {
    digitalWrite(LED_PIN, LOW);
    ledActive = false;
  }
  
  // Handle countdown timer
  if (totalSeconds > 0) {
    displayTime(totalSeconds);
    // If countdown is 10 seconds or less, beep the buzzer
    if (totalSeconds <= 10) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200); // Beep duration
      digitalWrite(BUZZER_PIN, LOW);
    }
    totalSeconds--;
    delay(1000); // Update every second
  } else {
    // Show "00:00" and stop countdown
    display.showNumberDecEx(0, 0b01000000, true);
    // Long final beep
    digitalWrite(BUZZER_PIN, HIGH);
    delay(1000);
    digitalWrite(BUZZER_PIN, LOW);
    
    // Wait for RFID scan to restart instead of halting
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Time's up!");
    lcd.setCursor(0, 1);
    lcd.print("Scan to restart");
    
    while (true) {
      // Handle web server clients in the waiting loop too
      server.handleClient();
      
      // Check if new card is present
      if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        // Reset timer and break the loop
        totalSeconds = 60;
        readRfidTag(); // Read and display the tag
        
        // Turn on LED when tag is scanned
        digitalWrite(LED_PIN, HIGH);
        ledActive = true;
        ledStartTime = millis();
        
        break;
      }
      
      // Check if LED should be turned off (if 2 seconds have passed)
      if (ledActive && (millis() - ledStartTime >= 2000)) {
        digitalWrite(LED_PIN, LOW);
        ledActive = false;
      }
      
      delay(100);
    }
  }
}

void displayTime(unsigned long totalSeconds) {
  int minutes = totalSeconds / 60;
  int seconds = totalSeconds % 60;
  uint8_t data[] = {
    display.encodeDigit(minutes / 10),
    display.encodeDigit(minutes % 10) | 0x80, // Colon
    display.encodeDigit(seconds / 10),
    display.encodeDigit(seconds % 10)
  };
  display.setSegments(data);
}

void checkRfid() {
  // Look for new cards
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    // Read RFID tag and reset timer
    readRfidTag();
    
    // Reset the timer to 60 seconds
    totalSeconds = 60;
    displayTime(totalSeconds);
    
    // Display on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Timer Reset: 60s");
    lcd.setCursor(0, 1);
    lcd.print("Tag: " + lastRfidTag);
    
    // Turn on LED when tag is scanned
    digitalWrite(LED_PIN, HIGH);
    ledActive = true;
    ledStartTime = millis();
    
    // Confirmation beep for timer reset
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void readRfidTag() {
  // Read RFID tag ID
  String rfidTag = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    rfidTag += (rfid.uid.uidByte[i] < 0x10 ? "0" : "") + String(rfid.uid.uidByte[i], HEX);
  }
  rfidTag.toUpperCase();
  
  // Check if this is a new tag or enough time has passed
  unsigned long currentTime = millis();
  if (rfidTag != lastRfidTag || (currentTime - lastTagTime >= TAG_COOLDOWN)) {
    lastRfidTag = rfidTag;
    lastTagTime = currentTime;
    
    // Print to serial for debugging
    Serial.print("RFID Tag: ");
    Serial.println(rfidTag);
  }
  
  // Halt PICC and stop encryption
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

// Web server route handlers
void handleRoot() {
  String html = "<html><head>";
  html += "<title>ESP32 Timer Control</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; text-align: center; }";
  html += "h1 { color: #0066cc; }";
  html += ".button { background-color: #4CAF50; color: white; padding: 10px 20px; ";
  html += "margin: 10px; border: none; border-radius: 5px; cursor: pointer; }";
  html += ".status { font-size: 24px; margin: 20px; }";
  html += "</style>";
  html += "<script>";
  html += "function updateStatus() {";
  html += "  fetch('/status').then(response => response.json())";
  html += "  .then(data => {";
  html += "    document.getElementById('timer').textContent = data.time;";
  html += "    document.getElementById('lastTag').textContent = data.tag;";
  html += "  });";
  html += "}";
  html += "setInterval(updateStatus, 1000);";
  html += "</script>";
  html += "</head><body>";
  html += "<h1>ESP32 Timer Control</h1>";
  html += "<div class='status'>Time: <span id='timer'>--:--</span></div>";
  html += "<div>Last RFID Tag: <span id='lastTag'>None</span></div>";
  html += "<button class='button' onclick='fetch(\"/reset\")'>Reset Timer</button>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleStatus() {
  int minutes = totalSeconds / 60;
  int seconds = totalSeconds % 60;
  String timeString = String(minutes < 10 ? "0" : "") + String(minutes) + ":" + 
                     String(seconds < 10 ? "0" : "") + String(seconds);
  
  String status = "{\"time\":\"" + timeString + "\",\"tag\":\"" + 
                 (lastRfidTag.length() > 0 ? lastRfidTag : "None") + "\"}";
  
  server.send(200, "application/json", status);
}

void handleReset() {
  totalSeconds = 60;
  
  // Display on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Timer Reset: 60s");
  lcd.setCursor(0, 1);
  lcd.print("Via Web Interface");
  
  // Confirmation beep for timer reset
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
  delay(100);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZER_PIN, LOW);
  
  server.send(200, "text/plain", "Timer reset");
}