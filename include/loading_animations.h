#ifndef LOADING_ANIMATIONS_H
#define LOADING_ANIMATIONS_H

#include <TFT_eSPI.h>

void animateBootLogo(TFT_eSPI &tft, int logoWidth, int logoHeight, const uint16_t *logoBitmap);
void animateLoadingBar(TFT_eSPI &tft, int x, int y, int w, int h, uint16_t color);

#endif