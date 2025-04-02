#include <TM1637Display.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>

// GPIO Definitions
const int CLK_PIN = 5;      // GPIO5 for TM1637 CLK
const int DIO_PIN = 4;      // GPIO4 for TM1637 DIO
const int BUZZER_PIN = 13;  // GPIO13 for buzzer
const int LED_PIN = 2;      // GPIO2 for LED
const int WIFI_LED_PIN = 16; // GPIO16 for WiFi LED
const int BRIGHTNESS_LEVEL = 2; // 0-7 brightness

// RFID Pins
#define SS_PIN  17  // GPIO17
#define RST_PIN 27  // GPIO27

// WiFi Settings
const char* serverSsid = "server_1st_esp32";
const char* serverPassword = "12345678";
unsigned long wifiConnectionAttemptTime = 0;
const unsigned long WIFI_RETRY_INTERVAL = 30000;
bool wifiConnected = false;

// Server Configuration
const char* serverAddress = "192.168.4.1";
const int serverPort = 80;

// System State
TM1637Display display(CLK_PIN, DIO_PIN);
MFRC522 rfid(SS_PIN, RST_PIN);
unsigned long totalSeconds = 60;
String lastRfidTag = "";
unsigned long lastTagTime = 0;
const unsigned long TAG_COOLDOWN = 3000;

// Timing Control
unsigned long previousMillis = 0;
const unsigned long timerInterval = 1000;
bool timerActive = true;

// LED/Buzzer Control
unsigned long ledStartTime = 0;
bool ledActive = false;
bool buzzerActive = false;
unsigned long buzzerStartTime = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize GPIO
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(WIFI_LED_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(WIFI_LED_PIN, LOW);
  
  // Initialize displays
  display.setBrightness(BRIGHTNESS_LEVEL);
  displayTime(totalSeconds);
  
  // Initialize RFID
  SPI.begin();
  rfid.PCD_Init();
  
  // Initialize WiFi
  WiFi.mode(WIFI_STA);
  connectToServerWiFi();
  
  Serial.println("System ready!");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // WiFi Management
  if (currentMillis - wifiConnectionAttemptTime >= WIFI_RETRY_INTERVAL) {
    checkWiFiConnection();
  }

  // RFID Handling
  checkRfid();

  // LED Timeout
  if (ledActive && (currentMillis - ledStartTime >= 2000)) {
    digitalWrite(LED_PIN, LOW);
    ledActive = false;
  }

  // Buzzer Timeout
  if (buzzerActive && (currentMillis - buzzerStartTime >= 200)) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerActive = false;
  }

  // Timer Handling
  if (timerActive) {
    if (currentMillis - previousMillis >= timerInterval) {
      previousMillis = currentMillis;
      
      if (totalSeconds > 0) {
        displayTime(totalSeconds);
        totalSeconds--;

        // Handle low time beep
        if (totalSeconds <= 10) {
          digitalWrite(BUZZER_PIN, HIGH);
          buzzerActive = true;
          buzzerStartTime = currentMillis;
        }
      } else {
        handleTimerExpiration();
      }
    }
  }
}

void displayTime(unsigned long totalSeconds) {
  int minutes = totalSeconds / 60;
  int seconds = totalSeconds % 60;
  uint8_t data[] = {
    display.encodeDigit(minutes / 10),
    display.encodeDigit(minutes % 10) | 0x80,
    display.encodeDigit(seconds / 10),
    display.encodeDigit(seconds % 10)
  };
  display.setSegments(data);
}

void checkRfid() {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    readRfidTag();
    
    if (wifiConnected) {
      sendRfidToServer(lastRfidTag);
    }

    totalSeconds = 60;
    displayTime(totalSeconds);
    
    // Visual/audio feedback
    digitalWrite(LED_PIN, HIGH);
    ledActive = true;
    ledStartTime = millis();
    
    // Double beep
    activateBuzzer(100);
    delay(150);
    activateBuzzer(100);
  }
}

void readRfidTag() {
  String rfidTag = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    rfidTag += (rfid.uid.uidByte[i] < 0x10 ? "0" : "") + String(rfid.uid.uidByte[i], HEX);
  }
  rfidTag.toUpperCase();
  
  unsigned long currentTime = millis();
  if (rfidTag != lastRfidTag || (currentTime - lastTagTime >= TAG_COOLDOWN)) {
    lastRfidTag = rfidTag;
    lastTagTime = currentTime;
    Serial.print("RFID Tag: ");
    Serial.println(rfidTag);
  }
  
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void handleTimerExpiration() {
    timerActive = false;
    display.showNumberDecEx(0, 0b01000000, true);
    // Removed the buzzer activation line: activateBuzzer(1000);
    
    while (true) {
      checkWiFiConnection();
      
      if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        totalSeconds = 60;
        readRfidTag();
        digitalWrite(LED_PIN, HIGH);
        ledActive = true;
        ledStartTime = millis();
        timerActive = true;
        previousMillis = millis();
        break;
      }
  
      if (ledActive && (millis() - ledStartTime >= 2000)) {
        digitalWrite(LED_PIN, LOW);
        ledActive = false;
      }
      
      delay(100);
    }
  }

void activateBuzzer(unsigned long duration) {
  digitalWrite(BUZZER_PIN, HIGH);
  buzzerActive = true;
  buzzerStartTime = millis();
}

// WiFi functions remain mostly the same as original, just removed LCD references
void connectToServerWiFi() {
  WiFi.begin(serverSsid, serverPassword);
  
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 10) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    digitalWrite(WIFI_LED_PIN, HIGH);
    wifiConnected = true;
  } else {
    Serial.println("\nWiFi connection failed");
    digitalWrite(WIFI_LED_PIN, LOW);
    wifiConnected = false;
  }
  
  wifiConnectionAttemptTime = millis();
}

void checkWiFiConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(WIFI_LED_PIN, LOW);
    wifiConnected = false;
    connectToServerWiFi();
  } else {
    digitalWrite(WIFI_LED_PIN, HIGH);
    wifiConnected = true;
  }
}

void sendRfidToServer(String rfidTag) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "http://" + String(serverAddress) + ":" + String(serverPort) + "/clientrfid";
    
    http.begin(url);
    http.addHeader("Content-Type", "text/plain");
    int httpCode = http.POST(rfidTag);
    
    Serial.print("HTTP Response: ");
    Serial.println(httpCode);
    
    http.end();
  }
}