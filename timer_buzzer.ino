#include <TM1637Display.h>

// GPIO Definitions for ESP32
const int CLK_PIN = 5;   // GPIO5 (previously G5)
const int DIO_PIN = 4;   // GPIO4 (previously G4)
const int BUZZER_PIN = 13; // Changed to GPIO13 to avoid conflict with DIO pin
const int BRIGHTNESS_LEVEL = 2; // Value between 0-7

TM1637Display display(CLK_PIN, DIO_PIN);
unsigned long totalSeconds = 60; // Start countdown from 1 minute

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Ensure it's off at the start
  display.setBrightness(BRIGHTNESS_LEVEL);
  displayTime(totalSeconds);
}

void loop() {
  if (totalSeconds > 0) {
    displayTime(totalSeconds);

    // If countdown is 10 seconds or less, beep the buzzer
    if (totalSeconds <= 10) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);                     // Beep duration
      digitalWrite(BUZZER_PIN, LOW);
    }

    totalSeconds--;
    delay(1000); // Update every second
  } else {
    // Show "00:00" and stop countdown
    display.showNumberDecEx(0, 0b01000000, true);

    // Long final beep
    digitalWrite(BUZZER_PIN, HIGH);
    delay(1000);
    digitalWrite(BUZZER_PIN, LOW);

    while (true); // Halt execution
  }
}

void displayTime(unsigned long totalSeconds) {
  int minutes = totalSeconds / 60;
  int seconds = totalSeconds % 60;

  uint8_t data[] = {
    display.encodeDigit(minutes / 10),
    display.encodeDigit(minutes % 10) | 0x80, // Colon
    display.encodeDigit(seconds / 10),
    display.encodeDigit(seconds % 10)
  };

  display.setSegments(data);
}