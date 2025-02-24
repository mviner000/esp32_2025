#include <SPI.h>
#include <MFRC522.h>

#define Buzzer 17   // Buzzer on GPIO17
#define SS_PIN 5    // RFID SDA (SS) on GPIO5
#define RST_PIN 22  // RFID RST on GPIO22

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

void setup() {
  Serial.begin(115200);
  SPI.begin();          // Initialize SPI bus
  mfrc522.PCD_Init();   // Initialize MFRC522
  pinMode(Buzzer, OUTPUT);
  Serial.println("Place RFID card near reader...");
}

void loop() {
  // Check for new RFID cards
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    // Print UID to Serial Monitor
    Serial.print("UID Tag:");
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println();

    // Activate buzzer for 1 second
    digitalWrite(Buzzer, HIGH);
    delay(1000);
    digitalWrite(Buzzer, LOW);

    // Halt PICC and stop encryption
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
  delay(50); // Add a small delay to reduce CPU load
}