// 2nd ESP32 (Client) 

#include <WiFi.h>
#include <SPI.h>
#include <MFRC522.h>

#define Buzzer 17
#define LED_PIN 16
#define WIFI_LED_PIN 15
#define SS_PIN 5
#define RST_PIN 22

// WiFi Configuration
const char* ssid = "ESP32-Server";
const char* password = "password123";

MFRC522 mfrc522(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);
  
  // Initialize WiFi
  pinMode(WIFI_LED_PIN, OUTPUT);
  digitalWrite(WIFI_LED_PIN, LOW);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  digitalWrite(WIFI_LED_PIN, HIGH);
  Serial.println("\nConnected to WiFi");

  // Initialize peripherals
  SPI.begin();
  mfrc522.PCD_Init();
  pinMode(Buzzer, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  // Existing RFID functionality
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uidString = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      uidString += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      uidString += String(mfrc522.uid.uidByte[i], HEX);
    }
    uidString.toUpperCase();
    
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
  }
  delay(50);
}