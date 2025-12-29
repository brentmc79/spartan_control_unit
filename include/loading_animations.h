#pragma once

#include <TFT_eSPI.h>

// Displays a boot logo with fade-in animation at screen center
void animateBootLogo(TFT_eSPI &tft, int logoWidth, int logoHeight, const uint16_t *logoBitmap);

// Draws an animated progress bar that fills from left to right
void animateLoadingBar(TFT_eSPI &tft, int x, int y, int w, int h, uint16_t color);