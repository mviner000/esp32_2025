#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>

// Set LCD parameters (0x27 is common I2C address, 16 columns, 2 rows)
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Adjust address if needed

// Set RFID pins
#define SS_PIN 5    // ESP32 GPIO5 to RFID SS/SDA
#define RST_PIN 27  // ESP32 GPIO27 to RFID RST
MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);
  
  // Initialize SPI bus
  SPI.begin();
  
  // Initialize RFID reader
  rfid.PCD_Init();
  Serial.println("RFID Reader initialized");
  
  // Initialize I2C LCD
  lcd.init();
  lcd.backlight();  // Turn on backlight
  
  // Display initial message
  lcd.setCursor(0, 0);
  lcd.print("RFID Scanner");
  lcd.setCursor(0, 1);
  lcd.print("Ready...");
}

void loop() {
  // Check if a new card is present
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String cardID = "";
    
    // Get RFID card ID
    for (byte i = 0; i < rfid.uid.size; i++) {
      cardID.concat(String(rfid.uid.uidByte[i] < 0x10 ? "0" : ""));
      cardID.concat(String(rfid.uid.uidByte[i], HEX));
    }
    cardID.toUpperCase();
    
    // Display card ID on Serial
    Serial.print("Card ID: ");
    Serial.println(cardID);
    
    // Display card ID on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Card Detected:");
    lcd.setCursor(0, 1);
    lcd.print(cardID);
    
    // Halt PICC and stop encryption
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    
    delay(2000);  // Display card ID for 2 seconds
  }
}