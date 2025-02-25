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
const char* serverIP = "192.168.4.1"; // Default IP of ESP32 AP
const int serverPort = 80;

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
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize peripherals
  SPI.begin();
  mfrc522.PCD_Init();
  pinMode(Buzzer, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

// Function to send RFID data to server
void sendRfidToServer(String rfidData) {
  WiFiClient client;
  
  Serial.println("Connecting to server...");
  if (client.connect(serverIP, serverPort)) {
    Serial.println("Connected to server");
    
    // Send HTTP request
    client.println("GET /send-rfid HTTP/1.1");
    client.println("Host: " + String(serverIP));
    client.println("Connection: close");
    client.println("RFID:" + rfidData);
    client.println();
    
    // Wait for response
    unsigned long timeout = millis();
    while (client.connected() && millis() - timeout < 3000) {
      if (client.available()) {
        String line = client.readStringUntil('\n');
        Serial.println(line);
      }
    }
    
    client.stop();
    Serial.println("Server connection closed");
  } else {
    Serial.println("Connection to server failed");
  }
}

void loop() {
  // RFID functionality with server communication
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uidString = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      uidString += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      uidString += String(mfrc522.uid.uidByte[i], HEX);
    }
    uidString.toUpperCase();
    
    Serial.print("UID Tag:");
    Serial.println(uidString);
    
    // Send the RFID data to the server
    sendRfidToServer(uidString);
    
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