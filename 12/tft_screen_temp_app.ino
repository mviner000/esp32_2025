#include <TFT_eSPI.h>

// Define bitmap data directly in the main file
// Temperature high icon
#define temp_high_width 32
#define temp_high_height 32
static const unsigned char temp_high[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00,
  0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00,
  0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x3C, 0x3C, 0x00,
  0x00, 0x7E, 0x7E, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x80, 0xFF, 0xFF, 0x01,
  0xC0, 0xFF, 0xFF, 0x03, 0xE0, 0xFF, 0xFF, 0x07, 0xF0, 0xFF, 0xFF, 0x0F,
  0xF0, 0xFF, 0xFF, 0x0F, 0xF0, 0xFF, 0xFF, 0x0F, 0xF0, 0xFF, 0xFF, 0x0F,
  0xF0, 0xFF, 0xFF, 0x0F, 0xF0, 0xFF, 0xFF, 0x0F, 0xE0, 0xFF, 0xFF, 0x07,
  0xC0, 0xFF, 0xFF, 0x03, 0x80, 0xFF, 0xFF, 0x01, 0x00, 0xFF, 0xFF, 0x00,
  0x00, 0x7E, 0x7E, 0x00, 0x00, 0x3C, 0x3C, 0x00, 0x00, 0x18, 0x18, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Temperature low icon
#define temp_low_width 32
#define temp_low_height 32
static const unsigned char temp_low[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00,
  0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00,
  0x00, 0x18, 0x18, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x3C, 0x3C, 0x00,
  0x00, 0x7E, 0x7E, 0x00, 0x00, 0xDB, 0xDB, 0x00, 0x80, 0x81, 0x81, 0x01,
  0xC0, 0x00, 0x00, 0x03, 0xE0, 0x00, 0x00, 0x07, 0xF0, 0x00, 0x00, 0x0F,
  0xF0, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x0F,
  0xF0, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x0F, 0xE0, 0x00, 0x00, 0x07,
  0xC0, 0x00, 0x00, 0x03, 0x80, 0x81, 0x81, 0x01, 0x00, 0xDB, 0xDB, 0x00,
  0x00, 0x7E, 0x7E, 0x00, 0x00, 0x3C, 0x3C, 0x00, 0x00, 0x18, 0x18, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

TFT_eSPI tft = TFT_eSPI(320, 240);
TFT_eSprite smaint = TFT_eSprite(&tft);   // Sprite object smaint
TFT_eSprite smainp = TFT_eSprite(&tft);   // Sprite object smainp
TFT_eSprite smaini = TFT_eSprite(&tft);   // Sprite object smaini
TFT_eSprite smainw = TFT_eSprite(&tft);   // Sprite object smainw
TFT_eSprite smainpr = TFT_eSprite(&tft);  // Sprite object smainpr

int power_percent;
String power_string;
int real_temp;
int o = 14;
float input_voltage = 36.4;
int power = 200;
String voltage_str, power_str;
int presset_temp[] = { 300, 350, 400 };
int screen = 0;
int state = 1;
int set_temp = 350;
int state1 = 1;
String state1_str[] = { "OFF", "Heating", "Standby" };
int iron_type = 0;
String iron_type_str[] = { "T245", "T12" };


//==========================================================================================
void setup() {
  Serial.begin(115200);
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(0);
  tft.invertDisplay(false);
  
  smaint.createSprite(210, 85);
  smainp.createSprite(52, 225);
  smaini.createSprite(320, 15);
  smainw.createSprite(210, 80);
  smainpr.createSprite(200, 25);

  smaint.fillSprite(TFT_BLACK);
  smaint.setTextColor(TFT_CYAN, TFT_BLACK);

  smainp.fillSprite(TFT_BLACK);
  smainp.setTextColor(TFT_CYAN, TFT_BLACK);

  smaini.fillSprite(TFT_BLACK);
  smaini.setTextColor(TFT_WHITE, TFT_BLACK);

  smainw.fillSprite(TFT_BLACK);
  smainw.setTextColor(TFT_WHITE, TFT_BLACK);

  smainpr.fillSprite(TFT_BLACK);
  smainpr.setTextColor(TFT_WHITE, TFT_BLACK);
  
  Serial.printf("x%d, y%d", tft.height(), tft.width());  
}

//==========================================================================================
long prmillis1, prmillis2, prmillis3;
int i;
String data;
void loop() {
  if (screen == 0) Mainscreen();
}

void Mainscreen_temp() {
  real_temp = 499;//map(analogRead(26), 0, 4096, 0, 500);

  smaint.setTextSize(3); // Use a larger text size instead of custom font
  smaint.drawNumber(real_temp, 200, 42);
  if (real_temp < 100) smaint.fillRect(0, 0, 60, 85, TFT_BLACK);
  if (real_temp < 10) smaint.fillRect(60, 0, 80, 85, TFT_BLACK);

  smaint.setTextSize(1); // Use default text size for the degree symbol
  smaint.setTextDatum(MR_DATUM);
  smaint.drawString("o", 198, 3, 2);
  smaint.setTextSize(2); // Larger text for "C"
  smaint.drawString("C", 210, 9);

  smaint.pushSprite(o + 48, 110);
}

void Mainscreen_power() {
  power_percent = 100; //map(analogRead(26), 0, 4095, 0, 100);
  smainp.setTextColor(TFT_WHITE, TFT_BLACK);
  smainp.setTextSize(1); // Use default text size
  smainp.setTextDatum(MC_DATUM);
  smainp.drawRect(15, 15, 20, 180, 0x195AFF);
  smainp.fillRect(0, 200, 50, 25, TFT_BLACK);
  power_string = "";
  power_string += power_percent;
  power_string += "%";
  smainp.drawString(power_string, 25, 207);

  smainp.fillRectVGradient(16, 16, 18, 178, TFT_RED, TFT_BLUE);
  smainp.fillRect(16, 16, 18, map(power_percent, 0, 100, 178, 0), TFT_BLACK);

  smainp.pushSprite(0, 16);
}

void Mainscreen_info() {
  smaini.setTextDatum(TL_DATUM);
  smaini.setTextSize(1); // Use default text size
  voltage_str = "";
  voltage_str += input_voltage;
  voltage_str += "V";
  smaini.drawString(voltage_str, 0, 2);
  power_str = "";
  power_str += power;
  power_str += "W";
  smaini.drawString(power_str, 65, 2);
  smaini.setTextDatum(TC_DATUM);
  smaini.drawString("15:45", 160, 2);
  smaini.setTextDatum(TR_DATUM);
  smaini.drawString("v2.0", 320, 2);

  smaini.pushSprite(0, 0);
}

void Mainscreen_presset() {
  tft.drawLine(0, 15, 320, 15, TFT_DARKGREY);
  smainpr.drawRect(0, 0, 50, 25, 0x195AFF);
  smainpr.drawRect(67, 0, 50, 25, 0x195AFF);
  smainpr.drawRect(132, 0, 50, 25, 0x195AFF);
  smainpr.setTextColor(TFT_WHITE, TFT_BLACK);
  smainpr.setTextSize(1); // Use default text size
  smainpr.setTextDatum(MC_DATUM);
  smainpr.drawNumber(presset_temp[0], 25, 12);
  smainpr.drawNumber(presset_temp[1], 92, 12);
  smainpr.drawNumber(presset_temp[2], 157, 12);
  smainpr.pushSprite(72, 215);
}

void Mainscreen_widgets() {
  smainw.fillRect(100, 10, 2, 7, TFT_LIGHTGREY);
  smainw.fillRect(100, 21, 2, 7, TFT_LIGHTGREY);
  smainw.fillRect(100, 32, 2, 7, TFT_LIGHTGREY);
  smainw.fillRect(100, 43, 2, 7, TFT_LIGHTGREY);
  smainw.fillRect(100, 54, 2, 7, TFT_LIGHTGREY);
  smainw.fillRect(100, 65, 2, 7, TFT_LIGHTGREY);
  if (state == 1) smainw.drawXBitmap(0, 12, temp_high, temp_high_width, temp_high_height, TFT_BLACK, TFT_RED);
  if (state == 0) smainw.drawXBitmap(0, 12, temp_low, temp_low_width, temp_low_height, TFT_BLACK, TFT_BLUE);
  smainw.setTextDatum(MC_DATUM);
  smainw.setTextSize(1); // Use default text size
  smainw.drawString("SET", 54, 20);
  smainw.drawRect(30, 40, 50, 25, 0x195AFF);
  smainw.setTextSize(1); // Use default text size
  smainw.drawNumber(set_temp, 55, 52);
  smainw.setTextDatum(MC_DATUM);
  smainw.setTextSize(1); // Use default text size
  if (state1 == 0){smainw.setTextColor(TFT_BLUE); smainw.drawString(state1_str[state1], 160, 20);}
  if (state1 == 1){smainw.setTextColor(TFT_RED); smainw.drawString(state1_str[state1], 160, 20);}
  if (state1 == 2){smainw.setTextColor(TFT_GREEN); smainw.drawString(state1_str[state1], 160, 20);}
  smainw.setTextSize(1); // Use default text size
  smainw.setTextColor(TFT_WHITE);
  smainw.drawString(iron_type_str[iron_type], 160, 52);
  smainw.pushSprite(60, 20);
}

void Mainscreen() {
  if (millis() - prmillis2 >= 150) {
    prmillis2 = millis();
    Mainscreen_info();
  }
  if (millis() - prmillis1 >= 50) {
    prmillis1 = millis();
    Mainscreen_power();
    Mainscreen_temp();
  }
  if (millis() - prmillis3 >= 500) {
    prmillis3 = millis();
    Mainscreen_presset();
    Mainscreen_widgets();
  }
}