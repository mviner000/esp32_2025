#include <WiFi.h>
#include <TFT_eSPI.h>
#include <WebServer.h>
#include <HTTPClient.h>

// WiFi Settings
const char* serverSsid = "PRIMENET";
const char* serverPassword = "grabebilis2";
const int ledPin = 15;

// TFT and Joystick Settings
TFT_eSPI tft = TFT_eSPI(320, 240);
TFT_eSprite smaini = TFT_eSprite(&tft);    // Navbar sprite
TFT_eSprite menuSprite = TFT_eSprite(&tft); // Menu sprite

#define JOYSTICK_Y_PIN 34  // Y-axis pin
#define JOYSTICK_BTN 32    // Joystick button pin

const int itemHeight = 24;
const int margin = 2;
const int itemTotalHeight = itemHeight + margin;
const int visibleItems = 7;
int selectedIndex = 0;
int scrollOffset = 0;

enum AppState { MAIN_MENU, PRODUCTS_MENU, BROWSER };
AppState currentState = MAIN_MENU;

// Browser state tracking
struct {
  bool requestMade;
  String resultMessage;
  bool success;
} browserState;

String voltage_str, power_str;

struct MenuItem {
  String name;
  String price;
};

MenuItem mainMenuItems[2] = {
  {"1. View Products", ""},
  {"2. Check Internet", ""}
};

MenuItem productItems[9] = {
  {"1. French Fries", "$4.99"},
  {"2. Cheeseburger", "$6.99"},
  {"3. Chicken Wings", "$5.99"},
  {"4. Caesar Salad", "$7.49"},
  {"5. Pizza Slice", "$3.99"},
  {"6. Ice Cream", "$2.99"},
  {"7. Soft Drink", "$1.99"},
  {"8. Coffee", "$2.49"},
  {"9. Back", ""}
};

void connectToServerWiFi();
void drawCurrentScreen();
void handleJoystickInput();
void handleButtonPress();
void Mainscreen();
void Mainscreen_info();
void drawMainMenu();
void drawProductsMenu();
void drawBrowser();

void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  pinMode(JOYSTICK_BTN, INPUT_PULLUP);

  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(0);
  analogReadResolution(12);

  smaini.createSprite(320, 15);
  menuSprite.createSprite(290, 190);
  menuSprite.setTextSize(2);

  connectToServerWiFi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(ledPin, LOW);
    connectToServerWiFi();
  }

  Mainscreen();
  handleJoystickInput();
  handleButtonPress();
  drawCurrentScreen();
}

void connectToServerWiFi() {
  WiFi.begin(serverSsid, serverPassword);
  
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 10) {
    delay(500);
    retries++;
  }
  digitalWrite(ledPin, WiFi.status() == WL_CONNECTED ? HIGH : LOW);
}

void drawCurrentScreen() {
  switch(currentState) {
    case MAIN_MENU:
      drawMainMenu();
      break;
    case PRODUCTS_MENU:
      drawProductsMenu();
      break;
    case BROWSER:
      drawBrowser();
      break;
  }
}

void handleJoystickInput() {
  static unsigned long lastPressTime = 0;
  if (millis() - lastPressTime < 200) return;

  int val = analogRead(JOYSTICK_Y_PIN);
  int maxItems = (currentState == MAIN_MENU) ? 1 : 8;

  if (val < 1000) { // Up
    lastPressTime = millis();
    if (selectedIndex > 0) {
      selectedIndex--;
      if (selectedIndex < scrollOffset) scrollOffset = selectedIndex;
    }
  }
  else if (val > 3000) { // Down
    lastPressTime = millis();
    if (selectedIndex < maxItems) {
      selectedIndex++;
      if (selectedIndex >= scrollOffset + visibleItems) 
        scrollOffset = selectedIndex - visibleItems + 1;
    }
  }
}

void handleButtonPress() {
  static unsigned long lastPress = 0;
  if (millis() - lastPress < 500) return;
  
  if (digitalRead(JOYSTICK_BTN) == LOW) {
    lastPress = millis();
    
    switch(currentState) {
      case MAIN_MENU:
        if (selectedIndex == 0) {
          currentState = PRODUCTS_MENU;
          selectedIndex = 0;
          scrollOffset = 0;
        } else {
          currentState = BROWSER;
          // Reset browser state when entering
          browserState.requestMade = false;
          browserState.resultMessage = "";
          browserState.success = false;
        }
        break;
        
      case PRODUCTS_MENU:
        if (selectedIndex == 8) {
          currentState = MAIN_MENU;
          selectedIndex = 0;
        }
        break;
        
      case BROWSER:
        currentState = MAIN_MENU;
        break;
    }
  }
}

void drawMainMenu() {
  menuSprite.fillSprite(TFT_MAGENTA);
  menuSprite.setTextColor(TFT_WHITE, TFT_MAGENTA);
  
  for(int i = 0; i < 2; i++) {
    int y = i * itemTotalHeight;
    
    if(i == selectedIndex) {
      menuSprite.fillRoundRect(0, y, 290, itemHeight, 4, TFT_NAVY);
    }
    
    menuSprite.setTextDatum(TL_DATUM);
    menuSprite.drawString(mainMenuItems[i].name, 10, y + 5);
  }
  
  menuSprite.pushSprite(15, 30);
  tft.drawRoundRect(13, 28, 294, 194, 8, TFT_WHITE);
}

void drawProductsMenu() {
  menuSprite.fillSprite(TFT_MAGENTA);
  menuSprite.setTextColor(TFT_WHITE, TFT_MAGENTA);
  
  for(int i = 0; i < visibleItems; i++) {
    int actualIndex = i + scrollOffset;
    if(actualIndex >= 9) break;
    
    int y = i * itemTotalHeight;
    
    if(actualIndex == selectedIndex) {
      menuSprite.fillRoundRect(0, y, 290, itemHeight, 4, TFT_NAVY);
      menuSprite.drawTriangle(270, y + itemHeight/2 - 8,
                              270, y + itemHeight/2 + 8,
                              285, y + itemHeight/2, TFT_WHITE);
    }
    
    menuSprite.setTextDatum(TL_DATUM);
    menuSprite.drawString(productItems[actualIndex].name, 10, y + 5);
    menuSprite.setTextDatum(TR_DATUM);
    menuSprite.drawString(productItems[actualIndex].price, 280, y + 5);
  }
  
  menuSprite.pushSprite(15, 30);
  tft.drawRoundRect(13, 28, 294, 194, 8, TFT_WHITE);
}

void drawBrowser() {
  if (!browserState.requestMade) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(50, 100);
    tft.print("Checking Internet...");

    HTTPClient http;
    http.begin("http://example.com"); // Simpler and reliable HTTP endpoint
    http.setTimeout(5000); // 5-second timeout

    int httpCode = http.GET();
    browserState.success = (httpCode == HTTP_CODE_OK);
    browserState.resultMessage = browserState.success 
      ? "Internet Connected!" 
      : "Connection Failed!";
    browserState.requestMade = true;

    http.end();
  }

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(browserState.success ? TFT_GREEN : TFT_RED);
  tft.setTextSize(2);
  tft.setCursor(50, 100);
  tft.print(browserState.resultMessage);

  tft.setTextColor(TFT_CYAN);
  tft.setCursor(10, 200);
  tft.print("[BACK]");
}

void Mainscreen_info() {
  smaini.fillSprite(TFT_BLACK);
  smaini.setTextColor(TFT_WHITE, TFT_BLACK);
  smaini.setTextDatum(TL_DATUM);
  
  // Get and display RAM usage
  uint32_t usedRAM = ESP.getHeapSize() - ESP.getFreeHeap();
  float usedRAM_KB = usedRAM / 1024.0f;
  String ramString = String(usedRAM_KB, 2) + "KB";
  smaini.drawString(ramString, 0, 2);

  smaini.drawString("200W", 65, 2);
  smaini.drawString("15:45", 130, 2);
  
  // WiFi status indicator
  smaini.fillCircle(300, 7, 3, WiFi.status() == WL_CONNECTED ? TFT_GREEN : TFT_RED);
  smaini.drawString("v2.1", 240, 2);
  smaini.pushSprite(0, 0);
}

void Mainscreen() {
  static unsigned long prmillis2 = 0;
  if (millis() - prmillis2 >= 150) {
    prmillis2 = millis();
    Mainscreen_info();
  }
}