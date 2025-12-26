#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <loading_animations.h>
#include <unsc_logo.h>

#define LCD_MOSI 23 // ESP32 D23
#define LCD_SCLK 18 // ESP32 D18
#define LCD_CS   15 // ESP32 D15
#define LCD_DC    2 // ESP32 D2
#define LCD_RST   4 // ESP32 D4
#define LCD_BLK  32 // ESP32 D32

Adafruit_ST7789 tft = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);

void setup() {
  tft.init(170, 320);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  scrollLogo(&tft);
  //loadingAnimation2(&tft);
  //renderHaloLoadingAnimation(&tft);
  //loadingAnimation(&tft);
}

void loop() {
  // Your main program loop
}