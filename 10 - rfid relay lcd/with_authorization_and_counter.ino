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

// Maximum number of UUID entries to store
#define MAX_ENTRIES 20

// Authorized UUID (the only one that should trigger the relay)
#define AUTHORIZED_UUID "841F1005"

// LCD parameters
LiquidCrystal_I2C lcd(0x27, 16, 2);  // I2C address 0x27, 16 columns, 2 rows

// Initialize RFID reader
MFRC522 rfid(SS_PIN, RST_PIN);

// Structure to store UUID and scan count
struct RFIDEntry {
  String uuid;
  int count;
};

RFIDEntry entries[MAX_ENTRIES];
int entryCount = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize Relay pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Start with relay OFF (locked state)
  
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

  // Track scan count
  int currentCount = -1;  // -1 indicates UUID not found initially
  
  // Check existing entries
  for (int i = 0; i < entryCount; i++) {
    if (entries[i].uuid == uuid) {
      entries[i].count++;
      currentCount = entries[i].count;
      break;
    }
  }

  // Handle new entry
  if (currentCount == -1) {
    if (entryCount < MAX_ENTRIES) {
      entries[entryCount].uuid = uuid;
      entries[entryCount].count = 1;
      currentCount = 1;
      entryCount++;
    } else {
      currentCount = 0;  // 0 indicates storage full
    }
  }

  // Display UUID and scan count on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Card: " + uuid);
  lcd.setCursor(0, 1);
  
  if (currentCount == 0) {
    lcd.print("Scans: N/A");
  } else {
    lcd.print("Scans: " + String(currentCount));
  }

  // Check if the scanned card is the authorized UUID
  bool isAuthorized = (uuid == AUTHORIZED_UUID);

  // Print to serial monitor
  Serial.print("RFID UUID: ");
  Serial.println(uuid);
  Serial.print("Scan count: ");
  Serial.println(currentCount);
  
  // Only activate relay and LED if authorized UUID is scanned
  if (isAuthorized) {
    // Activate relay and LED
    digitalWrite(RELAY_PIN, LOW);  // Unlock
    digitalWrite(LED_PIN, HIGH);   // Turn on LED
    
    Serial.println("Authorized Card - Door Unlocked");
    
    // Keep the lock open for defined time
    delay(LOCK_OPEN_TIME);
    
    // Reset lock and LED
    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(LED_PIN, LOW);
    
    Serial.println("Door Locked");
  } else {
    // Show unauthorized message on LCD
    lcd.setCursor(0, 1);
    lcd.print("Unauthorized");
    
    Serial.println("Unauthorized Card - Access Denied");
    
    // Short delay for unauthorized cards
    delay(1000);
  }

  // Update LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RFID Lock System");
  lcd.setCursor(0, 1);
  lcd.print("Scan Your Card");
  
  // Cleanup RFID reader
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  rfid.PCD_Init();
}