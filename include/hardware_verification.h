#ifndef HARDWARE_VERIFICATION_H
#define HARDWARE_VERIFICATION_H

#include <Adafruit_ST7789.h>
#include <Adafruit_NeoPixel.h>

void verifyButtons(Adafruit_ST7789& tft);
void verifyLEDs(Adafruit_ST7789& tft, Adafruit_NeoPixel& pixels);
void verifyFans(Adafruit_ST7789& tft);
void verifyHardwareConnections(Adafruit_ST7789& tft, Adafruit_NeoPixel& pixels);

#endif // HARDWARE_VERIFICATION_H
