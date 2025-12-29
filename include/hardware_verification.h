#pragma once

#include <TFT_eSPI.h>
#include <Adafruit_NeoPixel.h>

// Interactive hardware test mode that guides user through verifying
// button wiring and LED connections. Enable via VERIFY_HARDWARE flag in main.cpp.
void verifyHardwareConnections(TFT_eSPI &tft, Adafruit_NeoPixel &pixels);
