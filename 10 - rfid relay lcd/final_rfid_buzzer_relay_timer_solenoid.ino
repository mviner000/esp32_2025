#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>

// RFID pins
#define SS_PIN  5     // ESP32 GPIO5 to RFID RC522 SDA (SS)
#define RST_PIN 27    // ESP32 GPIO27 to RFID RC522 RST

// Relay control pin
#define RELAY_PIN 26  // ESP32 GPIO26 to relay control input

// LED pin
#define LED_PIN 25    // ESP32 GPIO25 to LED

// Buzzer pin
#define BUZZER_PIN 15 // ESP32 GPIO15 for buzzer

// Lock activation time in milliseconds
#define LOCK_OPEN_TIME 3000

// Maximum number of UUID entries to store
#define MAX_ENTRIES 20

// Authorized UUID (the only one that should trigger the relay)
#define AUTHORIZED_UUID "841F1005"

// Buzzer tones and durations
#define CORRECT_TONE 1000   // Hz for correct card
#define INCORRECT_TONE 400  // Hz for incorrect card
#define CORRECT_DURATION 200 // ms for correct beep
#define INCORRECT_DURATION 150 // ms for incorrect beep
#define BEEP_DELAY 100      // ms delay between beeps

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
  
  // Initialize buzzer pin
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Initialize SPI bus
  SPI.begin();
  
  // Initialize RFID
  rfid.PCD_Init();
  
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

  // Print UUID and scan count to serial monitor
  Serial.print("Card: ");
  Serial.println(uuid);
  Serial.print("Scans: ");
  
  if (currentCount == 0) {
    Serial.println("N/A (Storage Full)");
  } else {
    Serial.println(currentCount);
  }

  // Check if the scanned card is the authorized UUID
  bool isAuthorized = (uuid == AUTHORIZED_UUID);
  
  // Handle authorized vs. unauthorized cards
  if (isAuthorized) {
    // Play correct sound - one beep
    playCorrectSound();
    
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
    // Play incorrect sound - three beeps
    playIncorrectSound();
    
    Serial.println("Unauthorized Card - Access Denied");
    
    // Short delay for unauthorized cards
    delay(1000);
  }
  
  // Cleanup RFID reader
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  rfid.PCD_Init();
}

// Function to play the correct sound (one beep)
void playCorrectSound() {
  tone(BUZZER_PIN, CORRECT_TONE, CORRECT_DURATION);
  delay(CORRECT_DURATION);
  noTone(BUZZER_PIN);
  
  Serial.println("Played correct sound");
}

// Function to play the incorrect sound (three beeps)
void playIncorrectSound() {
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, INCORRECT_TONE, INCORRECT_DURATION);
    delay(INCORRECT_DURATION);
    noTone(BUZZER_PIN);
    
    // Add delay between beeps if not the last beep
    if (i < 2) {
      delay(BEEP_DELAY);
    }
  }
  
  Serial.println("Played incorrect sound");
}