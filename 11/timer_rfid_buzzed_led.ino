// old project

#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <TM1637Display.h>

// RFID pins
#define SS_PIN  5     // ESP32 GPIO5 to RFID RC522 SDA (SS)
#define RST_PIN 27    // ESP32 GPIO27 to RFID RC522 RST

// Relay control pin
#define RELAY_PIN 26  // ESP32 GPIO26 to relay control input

// LED pin
#define LED_PIN 25    // ESP32 GPIO25 to LED

// Buzzer pin
#define BUZZER_PIN 15 // ESP32 GPIO15 to buzzer

// TM1637 pins - CHANGED to use GPIO pins that aren't already in use
#define CLK_PIN 32    // ESP32 GPIO32 to TM1637 CLK (changed from 18)
#define DIO_PIN 33    // ESP32 GPIO33 to TM1637 DIO (changed from 19)

// Buzzer tones and durations
#define CORRECT_TONE 1000   // Hz for correct card
#define INCORRECT_TONE 400  // Hz for incorrect card
#define CORRECT_DURATION 200 // ms for correct beep
#define INCORRECT_DURATION 150 // ms for incorrect beep
#define BEEP_DELAY 100      // ms delay between beeps

// Lock activation time in milliseconds
#define LOCK_OPEN_TIME 3000

// Maximum number of UUID entries to store
#define MAX_ENTRIES 20

// Authorized UUID (the only one that should trigger the relay)
#define AUTHORIZED_UUID "841F1005"

// Countdown time in seconds
#define COUNTDOWN_TIME 59

// Initialize RFID reader
MFRC522 rfid(SS_PIN, RST_PIN);

// Initialize TM1637 display
TM1637Display display(CLK_PIN, DIO_PIN);

// Structure to store UUID and scan count
struct RFIDEntry {
  String uuid;
  int count;
};

RFIDEntry entries[MAX_ENTRIES];
int entryCount = 0;

// Variables for countdown timer
unsigned long timerStartMillis = 0;
int remainingSeconds = 0;
bool timerActive = false;

void setup() {
  Serial.begin(115200);
  
  // Initialize Relay pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Start with relay OFF (locked state)
  
  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // LED off initially
  
  // Initialize Buzzer pin
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Initialize SPI bus
  SPI.begin();
  
  // Initialize RFID
  rfid.PCD_Init();
  
  // Initialize TM1637 Display
  display.setBrightness(7); // Maximum brightness (0-7)
  display.showNumberDecEx(0, 0x40, true); // Display 00:00 with colon
  
  Serial.println("RFID Lock System Ready");
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Update countdown timer if active
  if (timerActive) {
    // Calculate remaining seconds based on elapsed time
    unsigned long elapsedSeconds = (currentMillis - timerStartMillis) / 1000;
    
    // Check if timer has elapsed more seconds
    if (elapsedSeconds <= COUNTDOWN_TIME) {
      int newRemainingSeconds = COUNTDOWN_TIME - elapsedSeconds;
      
      // Only update display if seconds have changed
      if (newRemainingSeconds != remainingSeconds) {
        remainingSeconds = newRemainingSeconds;
        updateTimerDisplay(remainingSeconds);
        
        // Debug output
        Serial.print("Countdown: ");
        Serial.print(remainingSeconds);
        Serial.println(" seconds remaining");
      }
      
      // Check if timer has reached zero
      if (remainingSeconds <= 0) {
        timerActive = false;
        display.showNumberDecEx(0, 0x40, true); // Show 00:00
        Serial.println("Countdown timer ended");
      }
    }
  }
  
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

  // Print UUID and scan count to Serial
  Serial.print("Card: ");
  Serial.println(uuid);
  Serial.print("Scans: ");
  if (currentCount == 0) {
    Serial.println("N/A (storage full)");
  } else {
    Serial.println(currentCount);
  }

  // Check if the scanned card is the authorized UUID
  bool isAuthorized = (uuid == AUTHORIZED_UUID);

  // Print to serial monitor
  Serial.print("RFID UUID: ");
  Serial.println(uuid);
  Serial.print("Scan count: ");
  Serial.println(currentCount);
  
  // Play the appropriate sound based on authorization
  if (isAuthorized) {
    playCorrectSound();
    
    // Activate relay and LED
    digitalWrite(RELAY_PIN, LOW);  // Unlock
    digitalWrite(LED_PIN, HIGH);   // Turn on LED
    
    Serial.println("Authorized Card - Door Unlocked");
    
    // Restart the countdown timer
    startCountdown();
    
    // Keep the lock open for defined time
    delay(LOCK_OPEN_TIME);
    
    // Reset lock and LED
    digitalWrite(RELAY_PIN, HIGH);
    digitalWrite(LED_PIN, LOW);
    
    Serial.println("Door Locked");
  } else {
    // Play incorrect sound for unauthorized cards
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

// Function to start the 59-second countdown
void startCountdown() {
  timerStartMillis = millis();
  remainingSeconds = COUNTDOWN_TIME;
  timerActive = true;
  updateTimerDisplay(remainingSeconds);
  
  Serial.println("Countdown timer started/restarted at 59 seconds");
}

// Function to update the TM1637 display with the remaining time
void updateTimerDisplay(int seconds) {
  // Format as MM:SS (00:SS in this case)
  int minutes = seconds / 60;
  int displaySeconds = seconds % 60;
  
  // Calculate the 4 digits to display (MMSS format)
  int displayValue = minutes * 100 + displaySeconds;
  
  // Display with colon
  display.showNumberDecEx(displayValue, 0x40, true);
}

// Function to play the correct sound (one beep)
void playCorrectSound() {
  tone(BUZZER_PIN, CORRECT_TONE, CORRECT_DURATION);
  delay(CORRECT_DURATION);
  noTone(BUZZER_PIN);
  
  Serial.println("Played correct sound");
}

// Function to play the incorrect sound for unauthorized cards
void playIncorrectSound() {
  tone(BUZZER_PIN, INCORRECT_TONE);
  delay(INCORRECT_DURATION);
  noTone(BUZZER_PIN);
  delay(BEEP_DELAY);
  tone(BUZZER_PIN, INCORRECT_TONE);
  delay(INCORRECT_DURATION);
  noTone(BUZZER_PIN);
}