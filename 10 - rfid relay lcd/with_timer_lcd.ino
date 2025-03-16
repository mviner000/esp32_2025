#include <Wire.h>
#include <LiquidCrystal_I2C.h>
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

// TM1637 pins - CHANGED to use GPIO pins that aren't already in use
#define CLK_PIN 32    // ESP32 GPIO32 to TM1637 CLK (changed from 18)
#define DIO_PIN 33    // ESP32 GPIO33 to TM1637 DIO (changed from 19)

// Lock activation time in milliseconds
#define LOCK_OPEN_TIME 3000

// Maximum number of UUID entries to store
#define MAX_ENTRIES 20

// Authorized UUID (the only one that should trigger the relay)
#define AUTHORIZED_UUID "841F1005"

// Countdown time in seconds
#define COUNTDOWN_TIME 59

// LCD parameters
LiquidCrystal_I2C lcd(0x27, 16, 2);  // I2C address 0x27, 16 columns, 2 rows

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
  
  // Initialize SPI bus
  SPI.begin();
  
  // Initialize RFID
  rfid.PCD_Init();
  
  // Initialize I2C LCD
  lcd.init();
  lcd.backlight();  // Turn on backlight
  
  // Initialize TM1637 Display
  display.setBrightness(7); // Maximum brightness (0-7)
  display.showNumberDecEx(0, 0x40, true); // Display 00:00 with colon
  
  // Display welcome message
  lcd.setCursor(0, 0);
  lcd.print("RFID Lock System");
  lcd.setCursor(0, 1);
  lcd.print("Scan Your Card");
  
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
    
    // Restart the countdown timer
    startCountdown();
    
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