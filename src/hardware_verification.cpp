#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include <hardware_verification.h>
#include <pins.h>

void verifyButtons(Adafruit_ST7789& tft) {
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
      }
      else {
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

void verifyLEDs(Adafruit_ST7789& tft, Adafruit_NeoPixel& pixels) {
  tft.fillScreen(ST77XX_BLACK);
  tft.fillRoundRect(10, 10, 145, 50, 5, ST77XX_WHITE);
  tft.fillRoundRect(165, 10, 145, 50, 5, ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_BLACK);
  tft.setCursor(50, 28);
  tft.println("LED 1");
  tft.setCursor(220, 28);
  tft.println("LED 2");
  int btn1Val = digitalRead(BUTTON_1);
  int btn2Val = digitalRead(BUTTON_2);
  int led1Color = 0;
  int led2Color = 0;

  int colors[5][3] = {
    {0, 0, 0}, 
    {0, 0, 255},
    {0, 255, 0},
    {255, 0, 0},
    {255, 255, 255}
  };

  pixels.setPixelColor(0, pixels.Color(colors[led1Color][0], colors[led1Color][1], colors[led1Color][2]));
  pixels.setPixelColor(1, pixels.Color(colors[led2Color][0], colors[led2Color][1], colors[led2Color][2]));
  pixels.setBrightness(10);
  pixels.show();

  while (true) {
    delay(50);
    int newBtn1Val = digitalRead(BUTTON_1);
    int newBtn2Val = digitalRead(BUTTON_2);
    if (newBtn1Val != btn1Val) {
      if (newBtn1Val == LOW) { //Button down
        tft.fillRoundRect(11, 11, 143, 48, 5, ST77XX_BLACK);
        tft.setTextColor(ST77XX_WHITE);
      } else { //Button up
        tft.fillRoundRect(11, 11, 143, 48, 5, ST77XX_WHITE);
        tft.setTextColor(ST77XX_BLACK);
        if (led1Color < 4)
          led1Color++;
        else
          led1Color = 0;
        pixels.setPixelColor(0, pixels.Color(colors[led1Color][0], colors[led1Color][1], colors[led1Color][2]));
        pixels.show();
      }
      tft.setCursor(50, 28);
      tft.println("LED 1");
      btn1Val = newBtn1Val;
    }
    if (newBtn2Val != btn2Val) {
      if (newBtn2Val == LOW) {
        tft.fillRoundRect(166, 11, 143, 48, 5, ST77XX_BLACK);
        tft.setTextColor(ST77XX_WHITE);
      } else {
        tft.fillRoundRect(166, 11, 143, 48, 5, ST77XX_WHITE);
        tft.setTextColor(ST77XX_BLACK);
        if (led2Color < 4)
          led2Color++;
        else
          led2Color = 0;
        pixels.setPixelColor(1, pixels.Color(colors[led2Color][0], colors[led2Color][1], colors[led2Color][2]));
        pixels.show();
      }
      tft.setCursor(220, 28);
      tft.println("LED 2");
      btn2Val = newBtn2Val;
    }
    if (btn1Val == LOW && btn2Val == LOW) {
      //pixels.setPixelColor(0, pixels.Color(0, 0, 0));
      //pixels.setPixelColor(1, pixels.Color(0, 0, 0));
      pixels.clear();
      pixels.show();
      break;
    }
  }
}

void verifyFans(Adafruit_ST7789& tft) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(20, 20);
  tft.println("Verifying Fan 1 (GPIO26)");
  tft.setCursor(20, 40);
  tft.println("Press Button 1 to toggle fan");
  tft.setCursor(20, 60);
  tft.println("Press Button 2 to continue");

  pinMode(FAN_1_CTRL, OUTPUT);
  digitalWrite(FAN_1_CTRL, LOW); // Fan off initially

  bool fan_on = false;

  while (true) {
    if (digitalRead(BUTTON_1) == LOW) { // Button 1 pressed
      fan_on = !fan_on;
      digitalWrite(FAN_1_CTRL, fan_on ? HIGH : LOW);
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(20, 20);
      tft.println(fan_on ? "Fan 1 ON" : "Fan 1 OFF");
      delay(500); // Debounce and display time
    }
    if (digitalRead(BUTTON_2) == LOW) { // Button 2 pressed, continue
      digitalWrite(FAN_1_CTRL, LOW); // Ensure fan is off
      delay(200); // Debounce
      break;
    }
    delay(50);
  }
}

void verifyHardwareConnections(Adafruit_ST7789& tft, Adafruit_NeoPixel& pixels) {
  verifyButtons(tft);
  //verifyLEDs(tft, pixels);
  //verifyFans(tft);
}