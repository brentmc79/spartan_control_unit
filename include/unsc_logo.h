#ifndef UNSC_LOGO_H
#define UNSC_LOGO_H

#include <Adafruit_ST7789.h>

extern const unsigned char unsc_bitmap_unsc_logo_cropped[] PROGMEM;
extern const int unsc_bitmap_allArray_LEN;
extern const unsigned char* unsc_bitmap_allArray[];

void scrollLogo(Adafruit_ST7789* display);

#endif // UNSC_LOGO_H
