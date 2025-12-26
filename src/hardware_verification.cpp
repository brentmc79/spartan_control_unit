#include <hardware_verification.h>
#include <Arduino.h>
#include <pins.h>

void verifyHardwareConnections(Adafruit_ST7789& tft) {
  Serial.println("Starting hardware verification...");
  tft.fillScreen(ST77XX_BLACK);
  tft.fillRoundRect(10, 10, 145, 50, 5, ST77XX_WHITE);
  tft.fillRoundRect(165, 10, 145, 50, 5, ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_BLACK);
  tft.setCursor(35, 28);
  tft.println("Button 1");
  tft.setCursor(195, 28);
  tft.println("Button 2");
  int btn1Val = digitalRead(BUTTON_1);
  int btn2Val = digitalRead(BUTTON_2);

  while (true) {
    delay(50);
    int newBtn1Val = digitalRead(BUTTON_1);
    int newBtn2Val = digitalRead(BUTTON_2);
    if (newBtn1Val != btn1Val) {
      if (newBtn1Val == LOW) {
        tft.fillRoundRect(11, 11, 143, 48, 5, ST77XX_BLACK);
        tft.setTextColor(ST77XX_WHITE);
      } else {
        tft.fillRoundRect(11, 11, 143, 48, 5, ST77XX_WHITE);
        tft.setTextColor(ST77XX_BLACK);
      }
      tft.setCursor(35, 28);
      tft.println("Button 1");
      btn1Val = newBtn1Val;
    }
    if (newBtn2Val != btn2Val) {
      if (newBtn2Val == LOW) {
        tft.fillRoundRect(166, 11, 143, 48, 5, ST77XX_BLACK);
        tft.setTextColor(ST77XX_WHITE);
      } else {
        tft.fillRoundRect(166, 11, 143, 48, 5, ST77XX_WHITE);
        tft.setTextColor(ST77XX_BLACK);
      }
      tft.setCursor(195, 28);
      tft.println("Button 2");
      btn2Val = newBtn2Val;
    }
    if (btn1Val == LOW && btn2Val == LOW) {
      break;
    }
  }
}
