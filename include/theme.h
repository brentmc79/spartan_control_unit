#pragma once

#include <Arduino.h>

// 1. Structure to hold raw RGB values
struct RGBColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

// 2. Define the Palette
const RGBColor COLOR_BG       = { 11,  13,  16 };   // Deep Charcoal
const RGBColor COLOR_BORDER   = { 0,   194, 212 };  // Electric Cyan (Active)
const RGBColor COLOR_TEXT_PRI = { 255, 255, 255 };  // White
const RGBColor COLOR_MUTED    = { 0,   39,  41 };   // Dark Green (Disabled/Inactive)

// 3. Utility: Convert to RGB565 (Standard for TFT_eSPI / Adafruit_GFX)
// Usage: tft.fillScreen(toRGB565(COLOR_BG));
inline uint16_t toRGB565(RGBColor color) {
    return ((color.r & 0xF8) << 8) | ((color.g & 0xFC) << 3) | (color.b >> 3);
}

// 4. Alternative: Pre-calculated 16-bit Hex (Faster, use these directly in tft functions)
#define HEX_BG       0x0862  // Deep Charcoal
#define HEX_BORDER   0x061A  // Electric Cyan
#define HEX_TEXT_PRI 0xFFFF  // White
#define HEX_MUTED    0x0145  // Dark Green