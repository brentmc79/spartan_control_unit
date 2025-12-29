#pragma once

#include <TFT_eSPI.h>

// UNSC logo bitmap data (100x57 pixels, 1-bit monochrome)
extern const unsigned char unsc_logo[];

// Renders UNSC logo with horizontal scroll animation
void drawUNSCLogo(TFT_eSPI &tft);
