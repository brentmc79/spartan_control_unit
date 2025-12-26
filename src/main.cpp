#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <loading_animations.h>
#include <unsc_logo.h>
#include <hardware_verification.h>
#include "pins.h"
#include <Adafruit_NeoPixel.h>

const bool VERIFY_HARDWARE = true;

Adafruit_ST7789 tft = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);
Adafruit_NeoPixel pixels(NUM_LEDS, LED_DATA, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);
  delay(100); // Allow pins to settle

  pixels.begin();
  pixels.clear();
  pixels.show();

  tft.init(170, 320);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);

  if (VERIFY_HARDWARE) {
    verifyHardwareConnections(tft, pixels);
  }

  scrollLogo(&tft);
  //loadingAnimation2(&tft);
  //renderHaloLoadingAnimation(&tft);
  //loadingAnimation(&tft);
}

void loop() {
  // Your main program loop
}