#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI(320, 240);
TFT_eSprite smaini = TFT_eSprite(&tft);   // Navbar sprite
TFT_eSprite quoteSprite = TFT_eSprite(&tft); // Quote sprite

String voltage_str, power_str;

// Improved function to draw wrapped text with better width utilization
void drawWrappedText(TFT_eSprite &sprite, String text, int x, int y, int w, int lineHeight) {
  int16_t cursor_x = x;
  int16_t cursor_y = y;
  String currentWord = "";
  
  sprite.setCursor(cursor_x, cursor_y);
  
  // Increase the usable width to utilize more of the available space
  int usableWidth = w + 30; // Increase the width constraint
  
  for (uint16_t i = 0; i < text.length(); i++) {
    char c = text[i];
    if (c == ' ' || c == '\n') {
      // Get width of the current word plus a space
      int16_t wordWidth = sprite.textWidth(currentWord + " ");
      
      // If adding this word would exceed the width, move to next line
      if (cursor_x + wordWidth > x + usableWidth) {
        cursor_x = x;
        cursor_y += lineHeight;
        sprite.setCursor(cursor_x, cursor_y);
      }
      
      // Print the word and space
      sprite.print(currentWord + " ");
      cursor_x += wordWidth;
      currentWord = "";
      
      // Handle explicit newlines
      if (c == '\n') {
        cursor_x = x;
        cursor_y += lineHeight;
        sprite.setCursor(cursor_x, cursor_y);
      }
    } else {
      currentWord += c;
    }
  }
  
  // Don't forget the last word
  if (currentWord.length() > 0) {
    int16_t wordWidth = sprite.textWidth(currentWord);
    if (cursor_x + wordWidth > x + usableWidth) {
      cursor_x = x;
      cursor_y += lineHeight;
      sprite.setCursor(cursor_x, cursor_y);
    }
    sprite.print(currentWord);
  }
}

//==========================================================================================
void setup() {
  Serial.begin(115200);
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(0);
  tft.invertDisplay(false);

  // Initialize navbar sprite
  smaini.createSprite(320, 15);
  smaini.fillSprite(TFT_BLACK);
  smaini.setTextColor(TFT_WHITE, TFT_BLACK);

  // Initialize quote sprite with updated dimensions
  quoteSprite.createSprite(290, 190);
  // Change background color to magenta
  quoteSprite.fillSprite(TFT_MAGENTA);
  // Change text color to white with magenta background
  quoteSprite.setTextColor(TFT_WHITE, TFT_MAGENTA);
  quoteSprite.setTextSize(2);

  // Create quote text
  String quote = "“You cant connect the dots looking forward; you can only connect them looking backwards. So you have to trust that the dots will somehow connect in your future.”";
  
  // Increase left margin by adjusting the x position (from 5 to 15)
  int leftMargin = 5;
  // Draw wrapped quote text with increased left margin
  drawWrappedText(quoteSprite, quote, leftMargin, 5, 280, 24); // Adjusted width to account for margin
  
  // Push quote to display with border
  // Move sprite position to the right to create left margin on screen
  quoteSprite.pushSprite(15, 30);
  // Adjust border rectangle to match the new dimensions and position
  tft.drawRoundRect(13, 28, 294, 194, 8, TFT_WHITE);
}

//==========================================================================================
void loop() {
  Mainscreen();
}

void Mainscreen_info() {
  smaini.fillSprite(TFT_BLACK);
  smaini.setTextDatum(TL_DATUM);
  smaini.setTextSize(1);
  voltage_str = "36.40V";
  smaini.drawString(voltage_str, 0, 2);
  power_str = "200W";
  smaini.drawString(power_str, 65, 2);
  smaini.setTextDatum(TC_DATUM);
  smaini.drawString("15:45", 160, 2);
  smaini.setTextDatum(TR_DATUM);
  smaini.drawString("v2.0", 320, 2);
  smaini.pushSprite(0, 0);
}

void Mainscreen() {
  static unsigned long prmillis2 = 0;
  if (millis() - prmillis2 >= 150) {
    prmillis2 = millis();
    Mainscreen_info();
  }
}