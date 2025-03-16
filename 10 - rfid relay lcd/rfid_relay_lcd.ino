#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>

// RFID pins
#define SS_PIN  5     // ESP32 GPIO5 to RFID RC522 SDA (SS)
#define RST_PIN 27    // ESP32 GPIO27 to RFID RC522 RST

// Relay control pin
#define RELAY_PIN 26  // ESP32 GPIO26 to relay control input

// LED pin
#define LED_PIN 25    // ESP32 GPIO25 to LED

// Lock activation time in milliseconds
#define LOCK_OPEN_TIME 3000

// LCD parameters
LiquidCrystal_I2C lcd(0x27, 16, 2);  // I2C address 0x27, 16 columns, 2 rows

// Initialize RFID reader
MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {
  Serial.begin(115200);
  
  // Initialize Relay pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Start with relay ON (locked state)
  
  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // LED off initially
  
  // Initialize SPI bus
  SPI.begin();
  
  // Initialize RFID
  rfid.PCD_Init();
  
  // Initialize I2C LCD
  lcd.init();
  lcd.backlight();  // Turn on backlight
  
  // Display welcome message
  lcd.setCursor(0, 0);
  lcd.print("RFID Lock System");
  lcd.setCursor(0, 1);
  lcd.print("Scan Your Card");
  
  Serial.println("RFID Lock System Ready");
}

void loop() {
  // Check if new card is present
  if (!rfid.PICC_IsNewCardPresent())
    return;

  // Read the card
  if (!rfid.PICC_ReadCardSerial())
    return;

  // Get UUID
  String uuid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uuid += (rfid.uid.uidByte[i] < 0x10 ? "0" : "") + String(rfid.uid.uidByte[i], HEX);
  }
  uuid.toUpperCase();
  
  // Display UUID on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Card: " + uuid);
  
  // Activate solenoid lock and LED
  lcd.setCursor(0, 1);
  lcd.print("Door Unlocked!");
  
  // Turn OFF the relay to unlock (reversed logic)
  digitalWrite(RELAY_PIN, LOW);
  
  // Turn ON the LED
  digitalWrite(LED_PIN, HIGH);
  
  // Print to serial monitor
  Serial.print("RFID UUID: ");
  Serial.println(uuid);
  Serial.println("Door Unlocked");
  
  // Keep the lock open for defined time
  delay(LOCK_OPEN_TIME);
  
  // Turn ON the relay and OFF the LED
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(LED_PIN, LOW);
  
  // Update LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RFID Lock System");
  lcd.setCursor(0, 1);
  lcd.print("Scan Your Card");
  
  Serial.println("Door Locked");
  
  // Cleanup RFID reader
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  
  // Critical fix: Reset and reinitialize RFID reader
  rfid.PCD_Init();
}