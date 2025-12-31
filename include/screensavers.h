#pragma once

#include <TFT_eSPI.h>

// Initialize screen saver state (call when entering screen saver mode)
void initMatrixScreenSaver();

// Render one frame of the Matrix rain animation
void renderMatrixScreenSaver(TFT_eSPI& tft);

// Initialize screen saver state for Biometric
void initBiometricScreenSaver();

// Render one frame of the Biometric screen saver
void renderBiometricScreenSaver(TFT_eSPI& tft);

// Initialize screen saver state for Radar
void initRadarScreenSaver();

// Render one frame of the Radar screen saver
void renderRadarScreenSaver(TFT_eSPI& tft);
