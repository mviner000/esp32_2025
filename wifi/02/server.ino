// 1st ESP32 (Server) 

#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define Buzzer 17
#define LED_PIN 16
#define SS_PIN 5
#define RST_PIN 22

// WiFi Configuration
const char* ssid = "ESP32-Server";
const char* password = "password123";
WiFiServer server(80);

// LCD Configuration
LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);
  
  // Initialize WiFi AP
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  // Initialize peripherals
  SPI.begin();
  mfrc522.PCD_Init();
  pinMode(Buzzer, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SSID: " + String(ssid));
  lcd.setCursor(0, 1);
  lcd.print("IP: " + IP.toString());
  delay(3000);
  
  // Start server
  server.begin();
  
  // Show ready message
  lcd.clear();
  lcd.print("RFID Reader");
  lcd.setCursor(0, 1);
  lcd.print(" Scan Card...");
}

void loop() {
  // Handle client connections
  WiFiClient client = server.available();
  if (client) {
    lcd.clear();
    lcd.print("Client Connected!");
    client.stop();
    delay(2000);
    lcd.clear();
    lcd.print("RFID Reader");
    lcd.setCursor(0, 1);
    lcd.print(" Scan Card...");
  }

  // Existing RFID functionality
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Card Detected!");
    
    String uidString = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      uidString += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      uidString += String(mfrc522.uid.uidByte[i], HEX);
    }
    uidString.toUpperCase();
    
    lcd.setCursor(0, 1);
    lcd.print("UID:");
    lcd.print(uidString);
    
    Serial.print("UID Tag:");
    Serial.println(uidString);
    
    digitalWrite(Buzzer, HIGH);
    digitalWrite(LED_PIN, HIGH);
    delay(1000);
    digitalWrite(Buzzer, LOW);
    digitalWrite(LED_PIN, LOW);
    
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    
    delay(2000);
    lcd.clear();
    lcd.print("RFID Reader");
    lcd.setCursor(0, 1);
    lcd.print(" Scan Card...");
  }
  delay(50);
}