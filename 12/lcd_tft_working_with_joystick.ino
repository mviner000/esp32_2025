#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI(320, 240);
TFT_eSprite smaini = TFT_eSprite(&tft);   // Navbar sprite
TFT_eSprite menuSprite = TFT_eSprite(&tft); // Menu sprite

#define JOYSTICK_Y_PIN 34  // Y-axis pin for up/down movement
#define JOYSTICK_X_PIN 35  // X-axis pin (not used in this code but defined for reference)

const int itemHeight = 24;
const int margin = 2;
const int itemTotalHeight = itemHeight + margin;
const int visibleItems = 7; // Corrected to fit 7 items in 190px height
int selectedIndex = 0;
int scrollOffset = 0;

String voltage_str, power_str;

struct MenuItem {
  String name;
  String price;
};

MenuItem menuItems[8] = {
  {"1. French Fries", "$4.99"},
  {"2. Cheeseburger", "$6.99"},
  {"3. Chicken Wings", "$5.99"},
  {"4. Caesar Salad", "$7.49"},
  {"5. Pizza Slice", "$3.99"},
  {"6. Ice Cream", "$2.99"},
  {"7. Soft Drink", "$1.99"},
  {"8. Coffee", "$2.49"}
};

void drawMenu() {
  menuSprite.fillSprite(TFT_MAGENTA);
  menuSprite.setTextColor(TFT_WHITE, TFT_MAGENTA);
  menuSprite.setTextSize(1);
  
  for(int i = 0; i < visibleItems; i++) {
    int actualIndex = i + scrollOffset;
    if(actualIndex >= 8) break;
    
    int y = i * itemTotalHeight;
    
    if(actualIndex == selectedIndex) {
      menuSprite.fillRoundRect(0, y, 290, itemHeight, 4, TFT_NAVY);
      menuSprite.drawTriangle(270, y + itemHeight/2 - 8,
                            270, y + itemHeight/2 + 8,
                            285, y + itemHeight/2, TFT_WHITE);
    }
    
    menuSprite.setTextDatum(TL_DATUM);
    menuSprite.drawString(menuItems[actualIndex].name, 10, y + 5);
    menuSprite.setTextDatum(TR_DATUM);
    menuSprite.drawString(menuItems[actualIndex].price, 280, y + 5);
  }
  
  menuSprite.pushSprite(15, 30);
  tft.drawRoundRect(13, 28, 294, 194, 8, TFT_WHITE);
}

void handleJoystickInput() {
  int val = analogRead(JOYSTICK_Y_PIN);
  static unsigned long lastPressTime = 0;
  
  if (millis() - lastPressTime < 200) return;
  
  if (val < 1000) { // Up movement (joystick values are inverted)
    lastPressTime = millis();
    if (selectedIndex > 0) {
      selectedIndex--;
      if (selectedIndex < scrollOffset) scrollOffset = selectedIndex;
    }
    drawMenu();
  }
  else if (val > 3000) { // Down movement
    lastPressTime = millis();
    if (selectedIndex < 7) {
      selectedIndex++;
      if (selectedIndex >= scrollOffset + visibleItems) 
        scrollOffset = selectedIndex - visibleItems + 1;
    }
    drawMenu();
  }
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(0);
  tft.invertDisplay(false);
  analogReadResolution(12);

  // Initialize navbar sprite
  smaini.createSprite(320, 15);
  smaini.fillSprite(TFT_BLACK);
  smaini.setTextColor(TFT_WHITE, TFT_BLACK);

  // Initialize menu sprite (now fits 8 items)
  menuSprite.createSprite(290, 190);
  menuSprite.setTextSize(2);

  drawMenu();
}

void loop() {
  Mainscreen();
  handleJoystickInput();
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