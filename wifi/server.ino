// server_1st_esp32

#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
1

#define Buzzer 17    // Buzzer on GPIO17
#define LED_PIN 16   // LED on GPIO16
#define SS_PIN 5     // RFID SDA (SS) on GPIO5
#define RST_PIN 22   // RFID RST on GPIO22

// Initialize LCD (I2C address, columns, rows)
LiquidCrystal_I2C lcd(0x27, 16, 2); // Change address if needed (0x3F is common)
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance

void setup() {
  Serial.begin(115200);
  SPI.begin();           // Initialize SPI bus
  mfrc522.PCD_Init();    // Initialize MFRC522
  
  pinMode(Buzzer, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // Ensure LED is off at start
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("RFID Reader");
  lcd.setCursor(0, 1);
  lcd.print(" Scan Card...");
  
  Serial.println("Place RFID card near reader...");
}

void loop() {
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    // Clear LCD and display UID
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Card Detected!");
    
    // Convert UID to hexadecimal string
    String uidString = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      uidString += String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      uidString += String(mfrc522.uid.uidByte[i], HEX);
    }
    uidString.toUpperCase();
    
    // Display UID on second line
    lcd.setCursor(0, 1);
    lcd.print("UID:");
    lcd.print(uidString);
    
    // Serial print UID
    Serial.print("UID Tag:");
    Serial.println(uidString);
    
    // Activate buzzer and LED
    digitalWrite(Buzzer, HIGH);
    digitalWrite(LED_PIN, HIGH);  // Turn on LED
    delay(1000);
    digitalWrite(Buzzer, LOW);
    digitalWrite(LED_PIN, LOW);   // Turn off LED
    
    // Halt PICC and stop encryption
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    
    // Reset LCD after 2 seconds
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RFID Reader");
    lcd.setCursor(0, 1);
    lcd.print(" Scan Card...");
  }
  delay(50);
}