// esp32_client_code
#include <TM1637Display.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>

// GPIO Definitions for ESP32
const int CLK_PIN = 5;      // GPIO5 for TM1637 CLK
const int DIO_PIN = 4;      // GPIO4 for TM1637 DIO
const int BUZZER_PIN = 13;  // GPIO13 for buzzer
const int LED_PIN = 2;      // GPIO2 for the new LED
const int WIFI_LED_PIN = 16; // GPIO16 for WiFi connection LED (changed from GPIO12)
const int BRIGHTNESS_LEVEL = 2; // Value between 0-7

// RFID Module Pins - Changed SS_PIN to avoid conflict
#define SS_PIN  17  // Changed to GPIO17 to avoid conflict with TM1637 CLK
#define RST_PIN 27  // ESP32 GPIO27

// WiFi Settings
const char* serverSsid = "server_1st_esp32";  // Name of the server WiFi hotspot
const char* serverPassword = "12345678";      // Password for the server WiFi hotspot
unsigned long wifiConnectionAttemptTime = 0;
const unsigned long WIFI_RETRY_INTERVAL = 30000; // 30 seconds retry interval
bool wifiConnected = false;

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
  pinMode(LED_PIN, OUTPUT);     // Initialize the LED pin
  pinMode(WIFI_LED_PIN, OUTPUT); // Initialize the WiFi LED pin
  digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer is off at the start
  digitalWrite(LED_PIN, LOW);    // Ensure LED is off at the start
  digitalWrite(WIFI_LED_PIN, LOW); // Ensure WiFi LED is off at the start
  
  // Initialize 7-segment display
  display.setBrightness(BRIGHTNESS_LEVEL);
  displayTime(totalSeconds);
  
  // Initialize LCD
  lcd.init();                      
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Timer System");
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi");
  
  // Initialize RFID
  SPI.begin();
  rfid.PCD_Init();
  
  // Initialize WiFi in station mode
  WiFi.mode(WIFI_STA);
  connectToServerWiFi();
  
  // Update LCD based on WiFi connection
  if (wifiConnected) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected");
    lcd.setCursor(0, 1);
    lcd.print("RSSI: " + String(WiFi.RSSI()) + " dBm");
    delay(2000);
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connection");
    lcd.setCursor(0, 1);
    lcd.print("Failed - Retry...");
    delay(2000);
  }
  
  // Return to normal display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Timer System");
  lcd.setCursor(0, 1);
  lcd.print("Scan RFID tag");
  
  Serial.println("System ready!");
}

void loop() {
  // Periodically check and reconnect WiFi if needed
  checkWiFiConnection();
  
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
      // Check and reconnect WiFi if needed
      checkWiFiConnection();
      
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

// Connect to the server's WiFi access point
void connectToServerWiFi() {
  Serial.println("Scanning for server WiFi...");
  
  // Scan for networks
  int numNetworks = WiFi.scanNetworks();
  bool serverFound = false;
  
  for (int i = 0; i < numNetworks; i++) {
    if (WiFi.SSID(i) == serverSsid) {
      serverFound = true;
      Serial.println("Server WiFi found!");
      break;
    }
  }
  
  if (serverFound) {
    Serial.print("Connecting to ");
    Serial.println(serverSsid);
    
    WiFi.begin(serverSsid, serverPassword);
    
    // Wait up to 10 seconds for connection
    int connectionAttempt = 0;
    while (WiFi.status() != WL_CONNECTED && connectionAttempt < 20) {
      delay(500);
      Serial.print(".");
      connectionAttempt++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      
      // Turn on WiFi LED to indicate successful connection
      digitalWrite(WIFI_LED_PIN, HIGH);
      wifiConnected = true;
    } else {
      Serial.println("");
      Serial.println("WiFi connection failed");
      digitalWrite(WIFI_LED_PIN, LOW);
      wifiConnected = false;
    }
  } else {
    Serial.println("Server WiFi not found");
    digitalWrite(WIFI_LED_PIN, LOW);
    wifiConnected = false;
  }
  
  wifiConnectionAttemptTime = millis();
}

// Check and reconnect WiFi if needed
void checkWiFiConnection() {
  unsigned long currentTime = millis();
  
  // If WiFi is connected, verify it's still connected
  if (wifiConnected && WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost");
    digitalWrite(WIFI_LED_PIN, LOW);
    wifiConnected = false;
    wifiConnectionAttemptTime = currentTime;
  }
  
  // If WiFi is not connected and enough time has passed since last attempt, try to reconnect
  if (!wifiConnected && (currentTime - wifiConnectionAttemptTime >= WIFI_RETRY_INTERVAL)) {
    Serial.println("Attempting to reconnect to WiFi");
    connectToServerWiFi();
  }
  
  // Update WiFi LED status
  digitalWrite(WIFI_LED_PIN, wifiConnected ? HIGH : LOW);
}