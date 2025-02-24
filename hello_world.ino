#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Set LCD parameters (0x27 is common I2C address, 16 columns, 2 rows)
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Adjust address if needed

void setup() {
  // Initialize I2C LCD
  lcd.init();
  lcd.backlight();  // Turn on backlight
  
  // Display message
  lcd.setCursor(0, 0);        // Start at column 0, line 0
  lcd.print("hello world,");
  lcd.setCursor(0, 1);        // Move to column 0, line 1
  lcd.print("esp32");
}

void loop() {
  // Nothing needed here for static display
}
